import psycopg2
from datetime import datetime

# Connect to your postgres DB
conn = psycopg2.connect(
            dbname="cardata",
            user="",
            password=""
)
# Open a cursor to perform database operations
cur = conn.cursor()

# Execute a query
cur.execute("SELECT * FROM tripData")

# Retrieve query results
records = cur.fetchall()

for record in records:
	print(record)


