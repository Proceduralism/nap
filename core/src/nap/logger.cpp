#include "logger.h"

#include <iostream>
#include <utility/fileutils.h>

namespace nap
{

	LogMessage::LogMessage(const LogLevel& lvl, const std::string& msg)
		: mLevel(lvl), mMessage(msg), mTimeStamp(utility::getCurrentTime())
	{}

	std::string basicLogMessageFormatter(const LogMessage& msg)
	{
		return utility::stringFormat("[%s] %s", msg.level().name().c_str(), msg.text().c_str());
	}

	std::string timestampLogMessageFormatter(const LogMessage& msg)
	{
		return utility::timeFormat(msg.getTimestamp()) + " " + basicLogMessageFormatter(msg);
	}

	LogHandler::LogHandler()
		: mLevel(Logger::fineLevel()), mFormatter(&basicLogMessageFormatter)
	{
	}

	void LogHandler::setFormatter(LogMessageFormatter formatter)
	{
		assert(formatter != nullptr);
		mFormatter = formatter;
	}

	std::string LogHandler::formatMessage(LogMessage& msg)
	{
		return mFormatter(msg);
	}

	Logger::Logger() : mLevel(fineLevel())
	{
		initialize();
	}


	Logger::Logger(Logger const&) : mLevel(fineLevel())
	{
		initialize();
	}


	void Logger::initialize()
	{
		log.connect(onLogSlot);
		addHandler(std::make_unique<ConsoleLogHandler>());
	}


	void Logger::onLog(const LogMessage& message)
	{
		for (auto& handler : mHandlers)
		{
			if (message.level() >= handler->getLogLevel())
				handler->commit(message);
		}
	}


	void ConsoleLogHandler::commit(LogMessage message)
	{
		bool isError = message.level() >= Logger::errorLevel();

		mOutStreamMutex.lock();

		if (isError)
			std::cerr << formatMessage(message) << std::endl;
		else
			std::cout << formatMessage(message) << std::endl;

		mOutStreamMutex.unlock();
	}


	Logger& Logger::instance()
	{
		static Logger instance;
		return instance;
	}


	void Logger::addHandler(std::unique_ptr<LogHandler> handler)
	{
		mHandlers.emplace_back(std::move(handler));
	}


	void Logger::addFileHandler(const std::string& filename)
	{
		debug("Logging to file: %s", filename.c_str());
		instance().addHandler(std::make_unique<FileLogHandler>(filename));
	}


	void Logger::logToDirectory(const std::string& directory, const std::string& prefix)
	{
		// filename-safe time format
		std::string timeformat = "%Y-%m-%d_%H-%M-%S_%ms";
		std::string timestamp = utility::timeFormat(utility::getCurrentTime(), timeformat);
		std::string filename = utility::stringFormat("%s/%s_%s.log",
													 directory.c_str(),
													 prefix.c_str(),
													 timestamp.c_str());
		addFileHandler(filename);
	}


	FileLogHandler::FileLogHandler(const std::string& mFilename)
		: LogHandler(),  mFilename(mFilename)
	{
		// Set the default formatter to use a timestamp
		setFormatter(&timestampLogMessageFormatter);
		// Kick off the writing thread
		mWriteThread = std::make_unique<std::thread>(std::bind(&FileLogHandler::writeLoop, this));
	}


	void FileLogHandler::commit(LogMessage message)
	{
		// Put messages on the queue, as not to be blocked by writing
		mQueueMutex.lock();
		mMessages.emplace(message);
		mQueueMutex.unlock();
	}


	FileLogHandler::~FileLogHandler()
	{
		mRunning = false;
	}


	void FileLogHandler::writeLoop()
	{
		// TODO: Do rollover after x number of bytes
		std::string dirname = utility::getFileDir(mFilename);

		if (!utility::dirExists(dirname))
		{
			if (!utility::makeDirs(dirname)) {
				Logger::error("Failed to create directory: %s (%s)", dirname.c_str(), std::strerror(errno));
				return;
			}
		}

		std::ofstream stream;
		stream.open(mFilename);

		if (!stream.is_open())
		{
			Logger::error("Failed to open log file for writing: %s (%s)", mFilename.c_str(), std::strerror(errno));
			return;
		}

		std::queue<LogMessage> writeQueue;

		while (mRunning)
		{
			// Consume all messages from the main queue
			mQueueMutex.lock();
			while (!mMessages.empty())
			{
				writeQueue.emplace(mMessages.front());
				mMessages.pop();
			}
			mQueueMutex.unlock();

			// Dump messages to the stream
			while (!writeQueue.empty())
			{
				LogMessage msg = writeQueue.front();
				writeQueue.pop();
				if (stream.good())
				{
					stream << formatMessage(msg) << std::endl;
				}
				else
				{
					nap::Logger::error("Failed writing to log: %s (%s)", mFilename.c_str(), std::strerror(errno));
					break;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}

		stream.flush();
		stream.close();
	}

}
