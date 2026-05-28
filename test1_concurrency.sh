echo "======= TEST 1: Concurrency ======="

# 1 - start the server
./coordinator &
SERVER_PID=$! # $! is for last ran PID
sleep 3 # wait for it to set up

echo "Launching 30 producers simultaneously..."
PRODUCER_PIDS=""
for i in {1..30}; do
    ./producer &
    PRODUCER_PIDS="$PRODUCER_PIDS $!"
done

# Wait for all producers to stop
wait $PRODUCER_PIDS
echo "All producers are done."

# Graceful shutdown
kill -SIGINT $SERVER_PID # kill server with SIGINT
wait $SERVER_PID

echo "==================================="