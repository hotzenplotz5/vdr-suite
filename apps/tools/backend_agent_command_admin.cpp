#include "AccountabilityEventRepository.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "Database.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
RequestSecurityContext administrator(const std::string& backendId)
{
    RequestSecurityContext context;context.requestId=backendAgentGenerateOpaqueId("req_",8);context.correlationId=context.requestId;context.authenticationState=AuthenticationState::Authenticated;context.actor=ActorIdentity{"system:backend-agent-command-admin",ActorType::System,"Backend Agent command administration utility",true};context.device=DeviceIdentity{"device:backend-agent-command-admin",true};context.credential=CredentialIdentity{"credential:backend-agent-command-admin",true,false,false};context.grants.push_back(PermissionGrant{"role.admin",backendId});context.permissionGrantResolution=PermissionGrantResolutionState::Resolved;return context;
}
std::string escape(const std::string& value){std::ostringstream out;for(unsigned char c:value){if(c=='\\')out<<"\\\\";else if(c=='"')out<<"\\\"";else if(c>=0x20)out<<static_cast<char>(c);}return out.str();}
void usage(){std::cerr<<"usage: vdr-suite-backend-agent-command-admin [--database PATH] [--backend ID] (--status | --enqueue-probe [--deadline-seconds N] | --replay COMMAND_ID | --arm-lost-receipt-response | --arm-lost-result-response)"<<std::endl;}
}
int main(int argc,char** argv)
{
    std::string databasePath="/var/lib/vdr-suite/vdr-suite.db",backendId="default",replayId;int deadlineSeconds=300;enum class Action{None,Status,Enqueue,Replay,FaultReceipt,FaultResult};Action action=Action::None;
    for(int i=1;i<argc;++i){const std::string arg=argv[i];if(arg=="--database"&&i+1<argc)databasePath=argv[++i];else if(arg=="--backend"&&i+1<argc)backendId=argv[++i];else if(arg=="--deadline-seconds"&&i+1<argc)deadlineSeconds=std::atoi(argv[++i]);else if(arg=="--status")action=action==Action::None?Action::Status:Action::None;else if(arg=="--enqueue-probe")action=action==Action::None?Action::Enqueue:Action::None;else if(arg=="--replay"&&i+1<argc){action=action==Action::None?Action::Replay:Action::None;replayId=argv[++i];}else if(arg=="--arm-lost-receipt-response")action=action==Action::None?Action::FaultReceipt:Action::None;else if(arg=="--arm-lost-result-response")action=action==Action::None?Action::FaultResult:Action::None;else{usage();return 64;}}
    if(action==Action::None||!backendAgentCommandSafeIdentifier(backendId)||deadlineSeconds<30||deadlineSeconds>3600||(action==Action::Replay&&!backendAgentCommandSafeIdentifier(replayId))){usage();return 64;}
    Database database;if(!database.open(databasePath)){std::cerr<<"failed to open Backend Agent database"<<std::endl;return 74;}AccountabilityEventRepository accountability(database);BackendAgentRepository agents(database);BackendAgentCommandRepository commands(database);if(!accountability.ensureSchema()||!agents.ensureSchema()||!commands.ensureSchema()){std::cerr<<"failed to initialize Backend Agent command repositories"<<std::endl;return 74;}BackendAgentCommandDeliveryService service(commands,agents,accountability);const auto now=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if(action==Action::Status){const auto s=service.summaryForBackend(backendId);std::cout<<"{\"present\":"<<(s.present?"true":"false");if(s.present)std::cout<<",\"commandId\":\""<<escape(s.commandId)<<"\",\"commandType\":\""<<escape(s.commandType)<<"\",\"state\":\""<<escape(s.state)<<"\",\"receiptCategory\":\""<<escape(s.receiptCategory)<<"\",\"resultCategory\":\""<<escape(s.resultCategory)<<"\",\"dispatchState\":\""<<escape(s.dispatchState)<<"\",\"verificationState\":\""<<escape(s.verificationState)<<"\",\"backendGeneration\":"<<s.backendGeneration<<",\"claimEpoch\":"<<s.claimEpoch<<",\"deadline\":"<<s.deadline;std::cout<<"}"<<std::endl;return 0;}
    std::string reason;const auto context=administrator(backendId);
    if(action==Action::Enqueue){const auto a=service.assignProbe(context,backendId,now,now+deadlineSeconds,reason);if(!a){std::cerr<<reason<<std::endl;return 1;}std::cout<<"{\"commandId\":\""<<escape(a->commandId)<<"\",\"operationId\":\""<<escape(a->operationId)<<"\",\"jobId\":\""<<escape(a->jobId)<<"\",\"attemptId\":\""<<escape(a->attemptId)<<"\",\"claimEpoch\":"<<a->claimEpoch<<",\"requestFingerprint\":\""<<escape(a->requestFingerprint)<<"\"}"<<std::endl;return 0;}
    const bool ok=action==Action::Replay?service.requestReplay(context,backendId,replayId,now,reason):service.armFault(context,backendId,action==Action::FaultReceipt?"receipt":"result",now,reason);if(!ok){std::cerr<<reason<<std::endl;return 1;}std::cout<<"{\"accepted\":true,\"reasonCode\":\""<<escape(reason)<<"\"}"<<std::endl;return 0;
}
