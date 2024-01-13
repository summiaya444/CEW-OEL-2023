#!/bin/bash

GMAIL_USERNAME="irzasalahuddin27@gmail.com"
GMAIL_PASSWORD="apjf wmkx llhm ljsg"
TO_ADDRESS="summiaya3fatimah@gmail.com"
SUBJECT="Weekly Weather Report"
BODY_FILE="weekly_report.txt"

# Check if the file exists
if [ ! -f "weekly_report.txt" ]; then
    echo "Error: weekly_report.txt not found."
    exit 1
fi

# Use mutt to send the email
mutt -s "$SUBJECT" "$TO_ADDRESS" -e "set smtp_url=smtps://irzasalahuddin27@gmail.com:apjfwmkxllhmljsg@smtp.gmail.com:465" -a "$BODY_FILE" < "$BODY_FILE"
