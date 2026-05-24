#!/bin/bash

echo -e "\n=== Compiling Entire Project ==="
make clean
make all

echo -e "\n=== Cleaning up old directories ==="
rm -rf downtown uptown active_reports-* .monitor_pid

echo -e "\n=== 1. Generating Test Data via city_manager ==="
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

./city_manager --role inspector --user alice --add uptown <<EOF
45.1
12.8
lighting
1
Broken streetlamp in park
EOF

echo -e "\n=== 2. Running city_hub Interactive Tests ==="
# We pipe automated commands directly into city_hub
echo -e "calculate_scores downtown uptown\nstart_monitor\nsleep\nstart_monitor\nexit\n" | ./city_hub

echo -e "\n=== 3. Cleaning up loose background monitors (if any) ==="
if [ -f .monitor_pid ]; then
    kill -INT $(cat .monitor_pid)
    sleep 0.5
fi

echo -e "\nPhase 3 Test Complete! Check the Combined Workload Report above."
