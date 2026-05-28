echo "======= TEST 3: Graceful Shutdown and SO_REUSEADDR ======="

# start server in debug mode
./coordinator -d &
SERVER_PID=$!
sleep 3 # let him breathe

echo ""
echo ">>>>> Sending SIGINT to server (same as CTRL+C)..."
kill -SIGINT $SERVER_PID

# wait for server to die
wait $SERVER_PID
echo ">>>>> Server terminated."

echo ""
echo ">>>>> INSTANTLY trying to revive it on the same port..."
./coordinator -d & # somebody bring a defibrillator
NEW_PID=$!

# wait 3 seconds to see if it blows up or if it's actually awake
sleep 3

# Check if new process is still alive and well
if ps -p $NEW_PID
then
    echo -e "\033[0m" #reset color
    echo "Test successful. The server was restarted without throwing error 'Address already in use'."
    echo "SO_REUSEADDR works as intended."
    # Kill this server as well
    kill -SIGINT $NEW_PID
else
    echo -e "\033[0m"
    echo "Test failed. The server couldn't restart."
fi

wait $NEW_PID
echo "================================"