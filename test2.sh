#!/bin/bash

echo -e "\n=== [Setup] Ensuring data exists for Iteration 2 tests ==="
./city_manager --role manager --user alice --add downtown <<EOF
12.2
21.1
road
2
Road closed due to pothole
EOF

./city_manager --role inspector --user bob --add downtown <<EOF
12.5
21.4
lighting
1
Flickering streetlamp
EOF

echo -e "\n\n=== [TEST 6] View a Specific Report ==="
./city_manager --role inspector --user bob --view downtown 1

echo -e "\n=== [TEST 7] Update Threshold (Manager: Alice) ==="
./city_manager --role manager --user alice --update_threshold downtown 3
cat downtown/district.cfg

echo -e "\n=== [TEST 8] Attempt Update Threshold (Inspector: Bob - Should Fail) ==="
./city_manager --role inspector --user bob --update_threshold downtown 2

echo -e "\n=== [TEST 9] Filter Reports (Severity >= 2 AND Category == road) ==="

./city_manager --role inspector --user bob --filter downtown "severity:>=:2" "category:==:road"

echo -e "\n=== [TEST 10] Remove Report (Manager: Alice) ==="
echo "Size before removal:"
stat -c%s downtown/reports.dat
./city_manager --role manager --user alice --remove_report downtown 1

echo -e "\n=== [TEST 11] Verify Removal (List remaining reports) ==="
./city_manager --role manager --user alice --list downtown
echo "Size after removal (Should be exactly 1 struct smaller):"
stat -c%s downtown/reports.dat

echo -e "\n=== Test 2 Completed ==="
