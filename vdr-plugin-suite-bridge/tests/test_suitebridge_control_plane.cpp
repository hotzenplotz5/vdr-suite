#include "../suitebridge_control_plane.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <future>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
using namespace vdrsuite::agent::control;
namespace {
Response transact(const std::string&path,Operation op,std::uint64_t id,std::uint64_t deadline,const std::string&payload={}){
 int fd=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0);assert(fd>=0);sockaddr_un a{};a.sun_family=AF_UNIX;std::copy(path.begin(),path.end(),a.sun_path);a.sun_path[path.size()]='\0';assert(connect(fd,reinterpret_cast<const sockaddr*>(&a),sizeof(a))==0);
 Request q;q.operation=op;q.requestId=id;q.deadlineNanoseconds=deadline;q.payload=payload;std::vector<std::uint8_t>b;assert(encodeRequest(q,b));assert(send(fd,b.data(),b.size(),0)==static_cast<ssize_t>(b.size()));
 b.assign(MaximumResponseFrameBytes+1,0);ssize_t n=recv(fd,b.data(),b.size(),MSG_TRUNC);assert(n>0);b.resize(static_cast<std::size_t>(n));close(fd);Response r;std::string why;assert(decodeResponse(b,r,why));return r;
}}
int main(){
 std::string path="/tmp/vdr-suite-control-plane-"+std::to_string(getpid())+".sock";std::mutex m;std::condition_variable cv;bool firstStarted=false,releaseFirst=false;int executions=0;
 SuiteBridgeControlPlane server(path,1);assert(server.Start([&](Operation op,const std::string&){SuiteBridgeCommandResult r{true,250,"{}"};if(op==Operation::LiveStatus){std::unique_lock<std::mutex>l(m);++executions;if(executions==1){firstStarted=true;cv.notify_all();cv.wait(l,[&]{return releaseFirst;});}}return r;}));
 auto expired=transact(path,Operation::LiveCapability,1,monotonicNowNanoseconds()-1);assert(expired.result==Result::DeadlineExpired);
 auto longDeadline=monotonicNowNanoseconds()+2000000000ULL;
 auto first=std::async(std::launch::async,[&]{return transact(path,Operation::LiveStatus,2,longDeadline,"lease\nepoch");});
 {std::unique_lock<std::mutex>l(m);assert(cv.wait_for(l,std::chrono::seconds(1),[&]{return firstStarted;}));}
 auto shortDeadline=monotonicNowNanoseconds()+60000000ULL;
 auto queued=std::async(std::launch::async,[&]{return transact(path,Operation::LiveStatus,3,shortDeadline,"lease\nepoch");});
 std::this_thread::sleep_for(std::chrono::milliseconds(20));
 auto overloaded=transact(path,Operation::LiveStatus,4,longDeadline,"lease\nepoch");assert(overloaded.result==Result::Overloaded);
 std::this_thread::sleep_for(std::chrono::milliseconds(80));{std::lock_guard<std::mutex>l(m);releaseFirst=true;}cv.notify_all();
 assert(first.get().result==Result::Success);assert(queued.get().result==Result::DeadlineExpired);assert(executions==1);
 auto metrics=server.SnapshotMetrics();assert(metrics.overloaded>=1);assert(metrics.deadlineExpiredBeforeExecution>=2);assert(metrics.queueHighWaterMark==1);assert(metrics.executedByOperation[2]==1);
 server.Stop();assert(!server.Running());assert(access(path.c_str(),F_OK)!=0);std::puts("SuiteBridge local control-plane server tests passed");
}
