import psycopg2

# Connect to your postgres DB
conn = psycopg2.connect(
            dbname="cardata",
            user="",
            password=""
)
# Open a cursor to perform database operations
cur = conn.cursor()

# Execute a query
cur.execute("DELETE FROM tripData where vin='12345';")

conn.commit()

conn.close()
