// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _CORE_APPLICATION_HH
#define _CORE_APPLICATION_HH

//#include <iostream>
#include <string>
#include <vector>

#include "Component.hh"
#include "Statistics.hh"
#include <string>
#include <map>

namespace Core {
class MappedArchive;
class MappedArchiveReader;
class MappedArchiveWriter;

    /**
     * \mainpage SPRINT
     * \image html http://www-i6.informatik.rwth-aachen.de/Pictures/Logo_i6_blue_small.png
     * \section InformationResources Information Resources:
     *  - http://www-i6/trac/sprint/wiki
     *  - http://www-i6/i6wiki/Sprint
     *  - \ref todo
     *  - \ref bug
     */

    /**
     * Application class:
     *
     * The application class provides a suitable wrapper for
     *
     * - global initializations
     * - configuration
     * - C++-like entry and exit points
     *
     * The default configuration mechanism consists of
     *
     * -# preparsing the commandline arguments and look for parameter '--config'
     * -# reading the configuration file
     * -# parsing the environment variable
     * -# final parsing of the commandline arguments
     *
     * The default name of the configuration file is the application
     * title provided to the constructor concatenated by '.config'.  The
     * default name of the environment variable is the capitalized
     * application title.
     *
     * To build a concrete application, derive from Core::Application and
     * use the macro APPLICATION(ConcreteApplicationClass) to create the
     * class instance and the global entry point.
     * Individual modules have to be initialized in the application class
     * constructor using INIT_MODULE(Module). Initialization is required to
     * register Flow nodes etc.
     **/

    class Application :
		public Component
    {
    private:
		std::vector<std::string> lowLevelErrorMessages_;

    protected:
		static const ParameterBool paramLogConfiguration;
		static const ParameterBool paramLogResolvedResources;
		static const ParameterString paramConfig;
		static const ParameterString paramCacheArchiveFile;
		static const ParameterBool paramCacheArchiveReadOnly;
		static const ParameterBool paramHelp;

		static std::string path_;
		static std::string basename_;

		Channel::Manager *channelManager_;
		Core::Channel *debugChannel_;
		Core::XmlChannel *debugXmlChannel_;
		std::map<std::string, MappedArchive*> cacheArchives_;

		bool defaultLoadConfigurationFile_;
		bool defaultOutputXmlHeader_;
		std::string comment_;
		std::string configFileName_;
		Timer timer_;

		void logSystemInfo() const;
		void logVersion() const;
		void logResources() const;
		void logResourceUsage() const;
		void logMemoryUsage() const;
		MappedArchive* getCacheArchive(const std::string& archive);

		/** Opens channels and makes initial logs. */
		void openLogging();
		/** Closes down channels and makes final logs
		 *  @param configAvailable is used if this function is called from
		 *  the destructor of an application instance. In this case the global
		 *  static configuration database might already be destroyed.
		 */
		void closeLogging(bool configAvailable = true);

		static Application *app_;
		int run(const std::vector<std::string> &arguments);

		static bool setFromEnvironment(const std::string &variable);
		static bool setFromFile(const std::string &filename);
		static std::vector<std::string> setFromCommandline(const std::vector<std::string> &arguments);

    public:
		Application();
		virtual ~Application();

		/** Get the global instance of the application (Singleton) */
		static Application *us() {
			return app_;
		}

		/**
		 * Return the name of the environment variable.
		 * Overload this method to set the name of the environment variable.
		 **/
		virtual std::string getVariable() const;

		/**
		 * Return the applications basename
		 **/
		std::string getBaseName() const;

		/**
		 * Return the applications path
		 **/
		std::string getPath() const;

		/**
		 * Overload this method to describe the usage of the application
		 * and to give a (short) description of the application.
		 * Default return string is
		 *     "usage: " + getBaseName() + "[OPTION(S)]\n"
		 **/

		/**
		 * Return the usage of the application.
		 *
		 * Normally, that method does not have to be overloaded;
		 * overload getApplicationDescription() and getParamterDescription()
		 * and getDefaultParameterDescription() instead.
		 **/
		virtual std::string getUsage() const;

		virtual std::string getApplicationDescription() const;
		virtual std::string getParameterDescription() const;
		virtual std::string getDefaultParameterDescription() const;


		/**
		 * Errors messages are collected and later (during closeLogging()) printed
		 * to the error channel.
		 *
		 * Use this, when the error can not be reported with Core::Component::error().
		 * @param msg the error message
		 */
		void reportLowLevelError(const std::string &msg = "");

		/**
		 * Configure default behaviour of loading a configuration file.
		 * You should call this in the constructor of your application, but not
		 * after that, because configuration files are loaded before entering
		 * main.
		 * @param f set to false if you don't want configuration files to be loaded
		 **/
		void setDefaultLoadConfigurationFile(bool f);

		/**
		 * Configure default behaviour of adding a valid xml header to standard
		 * channels. You should call this in the constructor of your application,
		 * but not after that, because the xml header is output on creation of
		 * channels.
		 * @param f set to false if you don't want the xml header to be output.
		 **/
		void setDefaultOutputXmlHeader(bool f);

		/**
		 * Set the title of the application.  You should call this in the
		 * constructor of your application, but not after that, because the
		 * default configuration is not reparsed.
		 * @param t title of the application
		 **/
		void setTitle(const std::string &t);

		/**
		 * Set the set of possible comment characters for this application.
		 * Comment characters are configurable and global to an application.
		 * @param c string of possible comment characters.
		 **/
		void setCommentCharacters(const std::string &c);

		/**
		 * Return the string of comment characters.
		 **/
		const std::string& getCommentCharacters(void) const { return comment_; }

		/**
		 * Returns the debug binary outpput channel.
		 **/
		Core::Channel& debugChannel(void);

		/**
		 * Returns the debug XML channel.
		 **/
		Core::XmlChannel& debugXmlChannel(void);

		/**
		 * Returns a pointer to the cache-archive reader for the archive,
		 * and the specified item contained by the archive.
		 * Returns zero if the given cache-archive was not configured, or if
		 * the item doesn't exist in the archive.
		 *
		 * The archive has to be configured through "archive-name.file=X" configuration
		 * */
		MappedArchiveReader getCacheArchiveReader(const std::string& archive_name, const std::string& item);

		/**
		 * Returns a pointer to the cache-archive writer for the archive,
		 * and the specified item contained by the archive.
		 * Returns zero if the given cache-archive was not configured, or is configured read-only (default: false).
		 *
		 * The archive has to be configured through "archive-name.file=..." and "archive-name.read-only=..." configuration
		 * */
		MappedArchiveWriter getCacheArchiveWriter(const std::string& archive_name, const std::string& item);

		/**
		 * Main entry point for an application.
		 **/
		virtual int main(const std::vector<std::string> &arguments) = 0;

		/**
		 * Terminate program execution.
		 * This function never returns.
		 * @param status
		 **/
		void exit(int status);

		/** private **/
		static int main(int argc, char *argv[]);
    };

} // namespace Application

#define APPLICATION(A)								\
	int main(int argc, char *argv[]) {				\
		A app;										\
		return Core::Application::main(argc, argv); \
	}

/**
 * Initialize the module by calling the constructor of its class Module.
 * Modules are always singletons, therefore the initialization will
 * done only once.
 */
#define INIT_MODULE(m)							\
	m::Module::instance();

#endif // _CORE_APPLICATION_HH
