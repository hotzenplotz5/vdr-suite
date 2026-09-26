#ifndef VDR_SUITE_BRIDGE_CONTROL_PLANE_H
#define VDR_SUITE_BRIDGE_CONTROL_PLANE_H
#include "SuiteBridgeControlPlaneProtocol.h"
#include "suitebridge_command_result.h"
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
class SuiteBridgeControlPlane final {
public:
 using Operation=vdrsuite::agent::control::Operation;
 using Handler=std::function<SuiteBridgeCommandResult(Operation,const std::string&)>;
 using Logger=std::function<void(const std::string&)>;
 struct Metrics { std::array<std::uint64_t,7> admittedByOperation{};std::array<std::uint64_t,7> executedByOperation{};std::uint64_t rejected=0,overloaded=0,deadlineExpiredBeforeExecution=0,protocolMismatch=0,peerRejected=0,queueHighWaterMark=0,queueWaitNanoseconds=0,executionNanoseconds=0;std::size_t queueDepth=0;bool running=false;};
 explicit SuiteBridgeControlPlane(std::string socketPath="/run/vdr/vdr-suite-control/control.sock",std::size_t queueCapacity=16);
 ~SuiteBridgeControlPlane();
 bool Start(Handler handler,Logger logger={}); void Stop(); bool Running()const; Metrics SnapshotMetrics()const; const std::string& SocketPath()const{return socketPath_;}
private:
 struct Pending{int fd=-1;vdrsuite::agent::control::Request request;std::uint64_t admittedAtNanoseconds=0;};
 static std::size_t operationIndex(Operation);
 void acceptLoop();void workerLoop();bool validatePeer(int);bool receiveRequest(int,vdrsuite::agent::control::Request&,std::string&);
 void sendResult(int,const vdrsuite::agent::control::Request&,vdrsuite::agent::control::Result,int,const std::string&,const char*);
 void log(Operation,std::uint64_t,const char*)const;void closeQueued(vdrsuite::agent::control::Result,const char*);
 std::string socketPath_;std::size_t queueCapacity_;Handler handler_;Logger logger_;int listenFd_=-1;std::atomic<bool> running_{false},stopRequested_{false};std::thread acceptThread_,workerThread_;mutable std::mutex queueMutex_;std::condition_variable queueChanged_;std::deque<Pending> queue_;
 std::array<std::atomic<std::uint64_t>,7> admittedByOperation_{};std::array<std::atomic<std::uint64_t>,7> executedByOperation_{};std::atomic<std::uint64_t> rejected_{0},overloaded_{0},deadlineExpiredBeforeExecution_{0},protocolMismatch_{0},peerRejected_{0},queueHighWaterMark_{0},queueWaitNanoseconds_{0},executionNanoseconds_{0};
};
#endif
