#include <fstream>

#include "IrStd/IrStd.hpp"

#include "Trader/Server/Server.hpp"
#include "Trader/Server/EndPoint/Manager.hpp"
#include "Trader/Server/EndPoint/Exchange.hpp"
#include "Trader/Server/EndPoint/Strategy.hpp"

#include "Trader/Manager/Manager.hpp"

IRSTD_TOPIC_REGISTER(Trader, Server);
IRSTD_TOPIC_USE_ALIAS(TraderServer, Trader, Server);

// ---- Trader::Server::ServerREST --------------------------------------------

Trader::Server::ServerREST::ServerREST(const int port)
		: IrStd::ServerREST(port)
{
	IrStd::FileSystem::pwd(m_rootPath);
	IrStd::FileSystem::append(m_rootPath, "www");
}

void Trader::Server::ServerREST::routeNotFound(IrStd::ServerHTTP::Context& context)
{
	// Strip the query string if any
	std::string uri(context.getRequest().getURI(), 0, context.getRequest().getURI().find_first_of('?'));

	// Default URL
	if (uri.empty() || uri == "/")
	{
		uri.assign("/index.html");
	}

	if (uri.find("..") == std::string::npos)
	{
		// Construct the full path
		auto fullPath = m_rootPath;
		IrStd::FileSystem::append(fullPath, uri.c_str());

		if (IrStd::FileSystem::isFile(fullPath))
		{
			// Read the file type
			const auto pos = fullPath.find_last_of('.');
			const auto type = fullPath.substr(pos + 1);
			const auto pMimeType = IrStd::ServerImpl::MimeType::fromFileExtension(type.c_str());

			context.getResponse().addHeader("Content-Type", pMimeType);
			{
				std::ifstream ifs(fullPath);
				std::string content((std::istreambuf_iterator<char>(ifs)),
						(std::istreambuf_iterator<char>()));
				context.getResponse().setData(content.c_str(), content.size());
				IRSTD_LOG_TRACE(TraderServer, "Serving file: " << fullPath << " ("
						<< IrStd::Type::Memory(content.size()) << ") to " << context.getRequest().getIp());
			}
			return;
		}
	}

	// Otherwise file not found
	context.getResponse().setStatus(404);
}

// ---- Trader::Server --------------------------------------------------------

Trader::Server::Server(Manager& trader, const int port)
		: m_server(port)
		, m_threadId()
		, m_trader(trader)
{
}

void Trader::Server::serverThread()
{
	EndPoint::Exchange exchangeEndPoint(m_trader);
	EndPoint::Strategy strategyEndPoint(m_trader);
	EndPoint::Manager managerEndPoint(m_trader);

	exchangeEndPoint.setup(m_server);
	strategyEndPoint.setup(m_server);
	managerEndPoint.setup(m_server);

	// List all available traces
	m_server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/trace", [&](IrStd::ServerREST::Context& context) {

		IrStd::Json json;
		size_t index = 0;
		IrStd::TopicImpl::getList([&](const IrStd::TopicImpl& topic) {
			json.add(index++, topic.getStr());
		});
		context.getResponse().setData(json);
	});

	// Set trace level to a specific topic
	m_server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/trace/{UINT}/trace", [&](IrStd::ServerREST::Context& context) {

		const size_t topicIndex = context.getMatchAsUInt(0);
		size_t index = 0;
		IrStd::TopicImpl::getList([&](const IrStd::TopicImpl& topic) {
			if (topicIndex == index++)
			{
				IrStd::Logger::getDefault().addTopic(topic, IrStd::Logger::Level::Trace);
			}
		});
	});

	IRSTD_LOG_INFO(TraderServer, "REST API & HTTP Server started on port " << m_server.getPort()
			<< ", reading from '" << m_server.m_rootPath
			<< "', example of usage: 'curl http://localhost:" << m_server.getPort() << "/api/v1/trace'");

	m_server.start();
}

void Trader::Server::start()
{
	IRSTD_ASSERT(m_threadId == std::thread::id());
	m_threadId = IrStd::Threads::create("RESTApiServer", &Trader::Server::serverThread, this);
}

void Trader::Server::stop()
{
	IRSTD_ASSERT(m_threadId != std::thread::id());
	m_server.stop();
	IrStd::Threads::terminate(m_threadId);
}
