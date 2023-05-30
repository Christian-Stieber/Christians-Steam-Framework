#pragma once

#include <tuple>
#include <google/protobuf/service.h>
#include <boost/callable_traits/args.hpp>
#include <type_traits>

/************************************************************************/
/*
 * Lets try to get some basic info from protobuf service stuff
 */

/************************************************************************/
/*
 * As an example, if protobuf defines a service "Player", it will
 * generate a 
 *    class Player
 *
 * For an RPC function "GetGameBadgeLevels" on that service, this class
 * will have a virtual function like so:
 *    virtual void GetGameBadgeLevels(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
 *                                    const ::CPlayer_GetGameBadgeLevels_Request* request,
 *                                    ::CPlayer_GetGameBadgeLevels_Response* response,
 *                                    ::google::protobuf::Closure* done);
 *
 * Thus, we should be able to translate a service/method name to the
 * request and result types...
 */

/************************************************************************/

namespace SteamBot
{
	namespace ProtobufService
	{
		template <typename T> struct Info
		{
			typedef boost::callable_traits::args_t<T> arguments;

			template <size_t i> struct arg
			{
				typedef typename std::tuple_element<i, arguments>::type type;
			};

			static_assert(std::tuple_size<arguments>::value==5, "service call parameter cound mismatch");
			static_assert(std::is_same<typename arg<1>::type, ::google::protobuf::RpcController*>::value, "service call parameter type mismatch");
			static_assert(std::is_same<typename arg<4>::type, ::google::protobuf::Closure*>::value, "service call parameter type mismatch");

			typedef std::remove_const_t<std::remove_pointer_t<typename arg<2>::type>> RequestType;
			typedef std::remove_pointer_t<typename arg<3>::type> ResultType;
		};
	}
}

/************************************************************************/
/*
 * Given a service and method name, creates:
 *   namespace <service>
 *   {
 *      namespace <method>
 *      {
 *         typedef RequestType
 *         typedef ResultType
 *
 *         static std::shared_ptr<ResultType> run(AsyncJobs&, RequestType&&, boost::asio::yield_context)
 *      }
 *   }
 */

#define TYPEDEF_PROTOBUF_SERVICE_CALL(SERVICENAME, METHODNAME) \
	namespace SERVICENAME { \
	namespace METHODNAME { \
	typedef SteamBot::ProtobufService::Info<decltype(&::SERVICENAME::METHODNAME)>::RequestType RequestType; \
	typedef SteamBot::ProtobufService::Info<decltype(&::SERVICENAME::METHODNAME)>::ResultType ResultType; \
	inline static auto run(Steam::Client::AsyncJobs& asyncJobs, RequestType&& request, boost::asio::yield_context yield) { \
	return asyncJobs.run<ResultType>(#SERVICENAME "." #METHODNAME "#1", std::move(request), yield); \
	} } } static_assert(true, "semicolon after macro")
