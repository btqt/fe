class Mockwatchdog_client {
  public:
    MOCK_METHOD0(heartbeat, int());
    MOCK_METHOD1(HeartBeat_Internal, int(int));
};

Mockwatchdog_client * M_watchdog_client;

int heartbeat(void)
{
    return M_watchdog_client->heartbeat();
}

void HeartBeat_Ready(void)
{

}

void HeartBeat()
{

}

int HeartBeat_Internal(int timeoutsec)
{
    return M_watchdog_client->HeartBeat_Internal(timeoutsec);
}
