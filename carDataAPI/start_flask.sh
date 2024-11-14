#!/bin/bash

# Navigate to the directory of your Flask project
cd /home/pi/Projects/carDataAPI

# Activate the virtual environment
source /home/pi/Projects/carDataAPI/env/bin/activate

# Start Gunicorn with your Flask app (adjust the number of workers as needed)
gunicorn --workers 3 --bind 0.0.0.0:5000 app:app
