#!/bin/bash

echo -e "\n=== Cleaning up old test data ==="
rm -rf downtown active_reports-*

echo -e "\n=== [TEST 1] Add Report (Manager: Alice) ==="
./city_manager --role manager --user alice --add downtown <<EOF
12.2
21.1
road
2
Road closed due to pothole
EOF

echo -e "\n\n=== [TEST 2] Add Report (Inspector: Bob) ==="
./city_manager --role inspector --user bob --add downtown <<EOF
12.5
21.4
lighting
1
Flickering streetlamp
EOF

echo -e "\n\n=== [TEST 3] List Reports (Inspector: Bob) ==="
./city_manager --role inspector --user bob --list downtown

echo -e "\n=== [TEST 4] Checking Symlink & Directory Structure ==="
ls -l active_reports-downtown
ls -la downtown/

echo -e "\n=== [TEST 5] Checking Audit Log ==="
cat downtown/logged_district

echo -e "\n=== Test 1 Completed ==="
