#pragma once
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>
namespace vdrsuite::agent::control {
constexpr std::uint32_t Magic=0x56534350U;
constexpr std::uint16_t ProtocolMajor=1, ProtocolMinor=0;
constexpr std::size_t RequestHeaderBytes=32, ResponseHeaderBytes=32;
constexpr std::size_t MaximumRequestPayloadBytes=2048, MaximumResponsePayloadBytes=65536;
constexpr std::size_t MaximumRequestFrameBytes=RequestHeaderBytes+MaximumRequestPayloadBytes;
constexpr std::size_t MaximumResponseFrameBytes=ResponseHeaderBytes+MaximumResponsePayloadBytes;
enum class Operation:std::uint16_t { LiveCapability=1, LiveOpen=2, LiveStatus=3, LiveClose=4 };
enum class ServiceClass:std::uint16_t { CriticalControl=1 };
enum class Result:std::uint16_t { Success=0, NativeRejected=1, StaleRejected=2, Overloaded=3, DeadlineExpired=4, ProviderUnavailable=5, ProtocolMismatch=6, PeerRejected=7, InternalFailure=8 };
struct Request { std::uint16_t major=ProtocolMajor, minor=ProtocolMinor; Operation operation=Operation::LiveCapability; std::uint64_t requestId=0, deadlineNanoseconds=0; std::string payload; };
struct Response { std::uint16_t major=ProtocolMajor, minor=ProtocolMinor; Result result=Result::InternalFailure; std::uint64_t requestId=0; std::int32_t replyCode=0; std::string payload; };
inline const char* operationName(Operation op){ switch(op){case Operation::LiveCapability:return "live-capability";case Operation::LiveOpen:return "live-open";case Operation::LiveStatus:return "live-status";case Operation::LiveClose:return "live-close";} return "unknown"; }
inline bool knownOperation(Operation op){ return op==Operation::LiveCapability||op==Operation::LiveOpen||op==Operation::LiveStatus||op==Operation::LiveClose; }
inline ServiceClass serviceClass(Operation){ return ServiceClass::CriticalControl; }
inline std::uint64_t monotonicNowNanoseconds(){ timespec v{}; if(clock_gettime(CLOCK_MONOTONIC,&v)!=0)return 0; return static_cast<std::uint64_t>(v.tv_sec)*1000000000ULL+static_cast<std::uint64_t>(v.tv_nsec); }
inline void append16(std::vector<std::uint8_t>&o,std::uint16_t v){o.push_back((v>>8)&0xff);o.push_back(v&0xff);}
inline void append32(std::vector<std::uint8_t>&o,std::uint32_t v){o.push_back((v>>24)&0xff);o.push_back((v>>16)&0xff);o.push_back((v>>8)&0xff);o.push_back(v&0xff);}
inline void append64(std::vector<std::uint8_t>&o,std::uint64_t v){append32(o,static_cast<std::uint32_t>(v>>32));append32(o,static_cast<std::uint32_t>(v));}
inline bool read16(const std::vector<std::uint8_t>&d,std::size_t p,std::uint16_t&v){if(p+2>d.size())return false;v=(static_cast<std::uint16_t>(d[p])<<8)|d[p+1];return true;}
inline bool read32(const std::vector<std::uint8_t>&d,std::size_t p,std::uint32_t&v){if(p+4>d.size())return false;v=(static_cast<std::uint32_t>(d[p])<<24)|(static_cast<std::uint32_t>(d[p+1])<<16)|(static_cast<std::uint32_t>(d[p+2])<<8)|d[p+3];return true;}
inline bool read64(const std::vector<std::uint8_t>&d,std::size_t p,std::uint64_t&v){std::uint32_t h=0,l=0;if(!read32(d,p,h)||!read32(d,p+4,l))return false;v=(static_cast<std::uint64_t>(h)<<32)|l;return true;}
inline bool encodeRequest(const Request&r,std::vector<std::uint8_t>&o){if(!knownOperation(r.operation)||r.requestId==0||r.deadlineNanoseconds==0||r.payload.size()>MaximumRequestPayloadBytes)return false;o.clear();o.reserve(RequestHeaderBytes+r.payload.size());append32(o,Magic);append16(o,r.major);append16(o,r.minor);append16(o,static_cast<std::uint16_t>(r.operation));append16(o,static_cast<std::uint16_t>(serviceClass(r.operation)));append64(o,r.requestId);append64(o,r.deadlineNanoseconds);append32(o,static_cast<std::uint32_t>(r.payload.size()));o.insert(o.end(),r.payload.begin(),r.payload.end());return true;}
inline bool decodeRequest(const std::vector<std::uint8_t>&d,Request&r,std::string&why){if(d.size()<RequestHeaderBytes||d.size()>MaximumRequestFrameBytes){why="request_frame_size";return false;}std::uint32_t magic=0,n=0;std::uint16_t op=0,cls=0;if(!read32(d,0,magic)||!read16(d,4,r.major)||!read16(d,6,r.minor)||!read16(d,8,op)||!read16(d,10,cls)||!read64(d,12,r.requestId)||!read64(d,20,r.deadlineNanoseconds)||!read32(d,28,n)){why="request_header";return false;}r.operation=static_cast<Operation>(op);if(magic!=Magic){why="magic";return false;}if(r.major!=ProtocolMajor||r.minor>ProtocolMinor){why="version";return false;}if(!knownOperation(r.operation)||cls!=static_cast<std::uint16_t>(serviceClass(r.operation))){why="operation_registry";return false;}if(r.requestId==0||r.deadlineNanoseconds==0){why="identity_deadline";return false;}if(n>MaximumRequestPayloadBytes||d.size()!=RequestHeaderBytes+n){why="request_payload_size";return false;}r.payload.assign(reinterpret_cast<const char*>(d.data()+RequestHeaderBytes),n);return true;}
inline bool encodeResponse(const Response&r,std::vector<std::uint8_t>&o){if(r.requestId==0||r.payload.size()>MaximumResponsePayloadBytes)return false;o.clear();o.reserve(ResponseHeaderBytes+r.payload.size());append32(o,Magic);append16(o,r.major);append16(o,r.minor);append16(o,static_cast<std::uint16_t>(r.result));append16(o,0);append64(o,r.requestId);append32(o,static_cast<std::uint32_t>(r.replyCode));append32(o,static_cast<std::uint32_t>(r.payload.size()));append32(o,0);o.insert(o.end(),r.payload.begin(),r.payload.end());return true;}
inline bool decodeResponse(const std::vector<std::uint8_t>&d,Response&r,std::string&why){if(d.size()<ResponseHeaderBytes||d.size()>MaximumResponseFrameBytes){why="response_frame_size";return false;}std::uint32_t magic=0,code=0,n=0,tail=0;std::uint16_t result=0,reserved=0;if(!read32(d,0,magic)||!read16(d,4,r.major)||!read16(d,6,r.minor)||!read16(d,8,result)||!read16(d,10,reserved)||!read64(d,12,r.requestId)||!read32(d,20,code)||!read32(d,24,n)||!read32(d,28,tail)){why="response_header";return false;}if(magic!=Magic||r.major!=ProtocolMajor||r.minor>ProtocolMinor||reserved!=0||tail!=0){why="response_protocol";return false;}if(n>MaximumResponsePayloadBytes||d.size()!=ResponseHeaderBytes+n){why="response_payload_size";return false;}r.result=static_cast<Result>(result);r.replyCode=static_cast<std::int32_t>(code);r.payload.assign(reinterpret_cast<const char*>(d.data()+ResponseHeaderBytes),n);return true;}
} // namespace vdrsuite::agent::control
