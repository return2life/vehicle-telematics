import os
from flask import Flask, request, jsonify, send_file
import csv
import psycopg2
from psycopg2.extras import RealDictCursor
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

def get_db_connection():
    return psycopg2.connect(
        dbname="cardata",
        user="",
        password="",
    )

@app.route('/upload', methods=['POST'])
def upload_data():
    if request.is_json:
        data = request.json
        session_id = data['session_id']
        vin = data['VIN']
        tripData = data['data']
        newTrip = data['newTrip']
        
        # Split the string into lines (rows)
        lines = tripData.split('\n')

        directory = "/home/pi/Projects/carDataAPI/" + str(vin)

        if not os.path.exists(directory):
            os.makedirs(directory)

        with open(directory + "/" + str(session_id)+".csv", mode="a", newline='') as file:
            writer = csv.writer(file)
            # Write each line as a row in the CSV
            for line in lines:
                # Split each line by commas
                row = line.split(",")
                writer.writerow(row)

        if newTrip == True:
            print("adding to meta data")
            conn = get_db_connection()
            cursor = conn.cursor()
            # Insert the values into PostgreSQL
            cursor.execute("""
                INSERT INTO tripData (VIN, startTime, endTime, idleTime, maxSpeed, fuelEff)
                VALUES (%s, %s, NULL, NULL, NULL, NULL);
            """, (vin, session_id))
            conn.commit()

            conn.close()
    else:
        return jsonify({"error": "Request must be JSON"}), 400

    return jsonify({"status": "success"}), 200

# GET: Retrieve all VINs
@app.route('/vins', methods=['GET'])
def get_all_vins():
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT DISTINCT VIN FROM tripData;")
    vins = cursor.fetchall()
    conn.close()
    
    vin_list = [vin[0] for vin in vins]  # Convert to list of VINs
    return jsonify({"VINs": vin_list}), 200

# GET: Retrieve sessions by VIN and/or date range
@app.route('/sessions', methods=['GET'])
def get_sessions():
    vin = request.args.get('vin')
    start_date = request.args.get('start')
    end_date = request.args.get('end')
    
    conn = get_db_connection()
    cursor = conn.cursor(cursor_factory=RealDictCursor)
    
    if vin and start_date and end_date:
        # If VIN and date range are provided
        cursor.execute("""
            SELECT * FROM tripData 
            WHERE VIN = %s AND startTime BETWEEN %s AND %s
            ORDER BY startTime DESC;
        """, (vin, start_date, end_date))
    elif vin:
        # If only VIN is provided (last 10 trips)
        cursor.execute("""
            SELECT vin, to_char(startTime,'YYYY-MM-DD HH24:MI:SS') as starttime FROM tripData 
            WHERE VIN = %s 
            ORDER BY startTime DESC 
            LIMIT 10;
        """, (vin,))
    elif start_date and end_date:
        # If only date range is provided
        cursor.execute("""
            SELECT * FROM tripData 
            WHERE startTime BETWEEN %s AND %s
            ORDER BY startTime DESC;
        """, (start_date, end_date))
    else:
        return jsonify({"error": "VIN or date range must be provided"}), 400
    
    sessions = cursor.fetchall()
    conn.close()
    
    return jsonify({"sessions": sessions}), 200

# GET: Retrieve CSV file by VIN and start datetime
@app.route('/getfile', methods=['GET'])
def get_trip_file():
    vin = request.args.get('vin')
    start_time = request.args.get('startTime')
    
    if not vin or not start_time:
        return jsonify({"error": "VIN and startTime are required"}), 400

    file_path = f"/home/pi/Projects/carDataAPI/{vin}/{start_time}.csv"

    if not os.path.exists(file_path):
        return jsonify({"error": "File not found"}), 404
    

    # Read the CSV and convert it to JSON
    data = []
    with open(file_path, mode='r') as csvfile:
        csvreader = csv.reader(csvfile)
        for row in csvreader:
            # Assuming each row in the CSV contains time, latitude, longitude, and altitude
            try:
                data.append({
                    "time": row[0],
                    "latitude": row[1],
                    "longitude": row[2],
                    "altitude": row[3],
                    "vehicle_speed": row[4],
                    "engine_load": row[5],
                    "mass_airflow": row[6],
                    "engine_rpm": row[7],
                    "throttle_pos": row[8]
                })
            except:
                pass

    return jsonify(data), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
