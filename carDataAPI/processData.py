import os
import csv
from datetime import datetime
import psycopg2

# Function to calculate fuel efficiency (example formula using mass airflow and vehicle speed)
def calculate_fuel_efficiency(mass_airflow, vehicle_speed):
    if vehicle_speed == 0:
        return None  # Avoid division by zero
    # Example conversion formula (adjust as per your vehicle's specifics)
    return (14.7 * vehicle_speed) / mass_airflow

# Function to update PostgreSQL database
def update_database(vin, start_time, end_time, idle_time, max_speed, fuel_efficiency):
    try:
        # Connect to PostgreSQL database
        conn = psycopg2.connect(
            dbname="cardata",
            user="",
            password=""
        )
        cur = conn.cursor()
        
        if fuel_efficiency == None:
            fuel_efficiency = "Null"
        if end_time == None:
            end_time = "Null"

        # Update query (assuming the VIN and startTime already exist)
        query = """
        UPDATE tripData
        SET endTime = '%s', idleTime = %s, maxSpeed = %s, fuelEff = %s
        WHERE vin = '%s' AND startTime = '%s'
        """% (end_time, idle_time, max_speed, fuel_efficiency, vin, start_time)
        print(query)
        cur.execute(query)
        
        # Commit the transaction
        conn.commit()
        
        cur.close()
        conn.close()
        
        print(f"Database updated successfully for VIN: {vin} and startTime: {start_time}")
    except Exception as e:
        print(f"Error updating database: {e}")

# Function to fetch all records with end_time = None
def get_pending_records():
    try:
        conn = psycopg2.connect(
            dbname="cardata",
            user="",
            password=""
        )
        cur = conn.cursor()

        # Query to select rows with None for end_time
        query = "SELECT vin, startTime FROM tripData WHERE endTime IS NULL"
        cur.execute(query)
        records = cur.fetchall()
        
        cur.close()
        conn.close()
        
        return records
    except Exception as e:
        print(f"Error fetching pending records: {e}")
        return []

# Process CSV file
def process_csv(file_path, vin, start_time):
    idle_time = 0
    max_speed = 0
    total_mass_airflow = 0
    total_distance = 0
    end_time = None
    count = 0

    with open(file_path, mode='r') as csv_file:
        csv_reader = csv.reader(csv_file)
        previous_time = None
        previous_speed = 1

        for row in csv_reader:
            try:
                current_time = datetime.strptime(row[0], '%Y-%m-%d %H:%M:%S')
                # Update end time
                end_time = current_time
                vehicle_speed = int(row[4])
                mass_airflow = float(row[6])

                # Calculate idle time (vehicle speed = 0)
                if vehicle_speed == 0:
                    if previous_time and previous_speed == 0:
                        idle_time += (current_time - previous_time).total_seconds()

                # Track max speed
                max_speed = max(max_speed, vehicle_speed)

                # Calculate fuel efficiency (if speed > 0)
                if vehicle_speed > 0:
                    total_mass_airflow += mass_airflow
                    total_distance += vehicle_speed
                    count += 1

                previous_time = current_time
                previous_speed = vehicle_speed
            except:
                pass

    # Calculate average fuel efficiency
    fuel_efficiency = calculate_fuel_efficiency(total_mass_airflow / count, total_distance / count) if count > 0 else None

    # Update the database with calculated values
    update_database(vin, start_time, end_time, idle_time, max_speed, fuel_efficiency)

# Main function to process pending records
def process_pending_records():
    pending_records = get_pending_records()
    
    for record in pending_records:
        vin = record[0]
        start_time = record[1]
        
        # Convert start_time to string format for folder name
        start_time_str = start_time.strftime('%Y-%m-%d %H:%M:%S')
        
        # Build the file path
        file_path = f'/home/pi/Projects/carDataAPI/{vin}/{start_time_str}.csv'
        
        if os.path.exists(file_path):
            print(f"Processing file: {file_path}")
            process_csv(file_path, vin, start_time)
        else:
            print(f"File not found: {file_path}")

# Run the process
process_pending_records()
print("done")
