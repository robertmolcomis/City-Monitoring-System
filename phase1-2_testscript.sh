#!/bin/bash

echo -e "\n=== Compiling Project ==="
make clean
make all

echo -e "\n=== Cleaning up old directories ==="
rm -rf downtown uptown active_reports-* .monitor_pid

echo -e "\n=== Starting Monitor Process (Phase 2) ==="
# Run monitor in the background and save its Process ID
./monitor_reports &
MONITOR_PID=$!

# Give the monitor a brief moment to initialize and write its PID file
sleep 1 

echo -e "\n=== Testing ADD & Monitor Notification (Phases 1 & 2) ==="
echo "[Adding report to Downtown...]"
./city_manager --role manager --user alice --add downtown <<EOF
10.5
20.1
road
3
Huge pothole on main street
EOF

sleep 0.5

echo "[Adding second report to Downtown...]"
./city_manager --role inspector --user bob --add downtown <<EOF
10.6
20.2
flooding
2
Blocked drain causing water pooling
EOF

sleep 0.5

echo "[Adding report to Uptown...]"
./city_manager --role inspector --user charlie --add uptown <<EOF
45.1
12.8
lighting
1
Broken streetlamp in park
EOF

echo -e "\n=== Testing UPDATE_THRESHOLD (Phase 1) ==="
./city_manager --role manager --user alice --update_threshold downtown 2

echo -e "\n=== Testing LIST (Phase 1) ==="
./city_manager --role inspector --user bob --list downtown

echo -e "\n=== Testing FILTER (Phase 1) ==="
echo "Filtering for severity >= 2 in Downtown:"
./city_manager --role inspector --user bob --filter downtown 'severity:>=:2'

echo -e "\n=== Testing VIEW (Phase 1) ==="
./city_manager --role inspector --user bob --view downtown 1

echo -e "\n=== Testing REMOVE_REPORT (Phase 1) ==="
./city_manager --role manager --user alice --remove_report downtown 1

echo -e "\n=== Verifying Logged Actions & Monitor Success ==="
echo "Contents of downtown/logged_district:"
cat downtown/logged_district

echo -e "\n=== Testing REMOVE_DISTRICT (Phase 2) ==="
echo "Active symlinks BEFORE removal:"
ls -l active_reports-*
echo "Removing Uptown..."
./city_manager --role manager --user alice --remove_district uptown
echo "Active symlinks AFTER removal:"
ls -l active_reports-*

echo -e "\n=== Shutting Down Monitor Process (Phase 2) ==="
# Send SIGINT (Ctrl+C equivalent) to the monitor
kill -INT $MONITOR_PID
# Wait for it to finish its cleanup routine
wait $MONITOR_PID

echo -e "\n=== Final Directory Structure Generated ==="
echo "Contents of Downtown:"
ls -la downtown/

echo -e "\nCheck complete. Test script finished!"
