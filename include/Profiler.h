#pragma once


class Profiler {
public:
    void Start();
    void Stop();
    void Print();
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StopTime;
};
