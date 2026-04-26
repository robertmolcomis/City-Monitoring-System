#!/bin/bash

echo -e "\n=== Cleaning up old directories ==="
rm -rf downtown uptown active_reports-*

echo -e "\n=== Creating District 1: Downtown ==="
./city_manager --role manager --user alice --add downtown <<EOF
10.5
20.1
road
3
Huge pothole on main street
EOF

./city_manager --role inspector --user bob --add downtown <<EOF
10.6
20.2
flooding
2
Blocked drain causing water pooling
EOF

./city_manager --role manager --user alice --update_threshold downtown 2

echo -e "\n=== Creating District 2: Uptown ==="
./city_manager --role inspector --user charlie --add uptown <<EOF
45.1
12.8
lighting
1
Broken streetlamp in park
EOF

./city_manager --role manager --user alice --add uptown <<EOF
45.2
12.9
road
2
Cracked pavement
EOF

echo -e "\n=== Final Directory Structure Generated ==="
echo "List of all active_reports symlinks:"
ls -l active_reports-*

echo -e "\nContents of Downtown:"
ls -la downtown/

echo -e "\nContents of Uptown:"
ls -la uptown/

echo -e "\nSetup Complete! Your directories are ready for submission."
