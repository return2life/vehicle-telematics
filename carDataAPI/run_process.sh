#!/bin/bash

# Navigate to the directory of your Flask project
cd /home/pi/Projects/carDataAPI

# Activate the virtual environment
source /home/pi/Projects/carDataAPI/env/bin/activate

# Start Gunicorn with your Flask app (adjust the number of workers as needed)
python processData.py
