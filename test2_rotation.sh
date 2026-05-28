echo "======= TEST 2: Rotation ======="
rm -f logs/*.log

# start the server
./coordinator &
SERVER_PID=$!
sleep 3

echo "Launching 1500 producers"
for i in {1..1500}; do
    ./producer > /dev/null & sleep 0.001 # a little delay not to overwhelm this poor server
    # also redirect output in dev/null or outputs will be going CRAZY
done

echo "Waiting 4 seconds for SIGALARM..." # more than enough, it rings every second
sleep 4

# gracefully kill this poor server
kill -SIGINT $SERVER_PID
wait $SERVER_PID

echo "There should be at least one "archive" file in addition to a new current.log"
ls -l logs/
echo "================================"