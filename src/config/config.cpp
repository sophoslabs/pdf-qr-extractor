// Copyright (C) 2026 Sophos Limited
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This file is part of pdf-qr-extractor.
//
// pdf-qr-extractor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pdf-qr-extractor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdf-qr-extractor. If not, see <https://www.gnu.org/licenses/>

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <algorithm>
#include "config/config.h"
#ifdef __linux__
#  include <stdlib.h>	// getenv
#elif defined _WIN32
#  include <windows.h>  // GetEnvironmentVariable
#endif
#include <sys/stat.h>

namespace extractor::util {

Config::Config( const std::string& fn )
{
	if ( fn.empty() ) {
		return;
	}

	std::ifstream f;
	f.exceptions( std::ifstream::failbit | std::ifstream::badbit );

	try {
		struct stat st;
		stat(fn.c_str(), &st);
		if (st.st_mode & S_IFDIR){
			// Config file cannot be directory
			throw std::ifstream::failure("");
		}
		f.open( fn.c_str() );
		f >> *this;
		f.close();
	}
	catch (const std::ifstream::failure &e) {
		std::cerr << "Error: Config file '" << fn << "' cannot be opened. Default config is used." << std::endl;
	}
}

std::string Config::getEnvVariable( const std::string& key ) const
{
	char* val = NULL;
#if (defined WIN32 || defined WIN64)
	char buff[1024];
	val = buff;
	if ( GetEnvironmentVariable( key.c_str(), val, 1024 ) == 0 ) {
		val = NULL;
	}
#else
	val = getenv( key.c_str() );
#endif
	std::string retval = "";
	if (val != NULL) {
		retval = val;
	}
	else {
		throw std::runtime_error("Environment variable not found: " + key);
	}
	return retval;
}

bool Config::isKey( const std::string &s ) const
{
	return count( s ) != 0;
}

std::string Config::checkKey( const std::string &key ) const
{
	if ( !isKey( key ) ) {	// Does key exist?		
			throw std::out_of_range("Key not found: " + key);	// no default key!		
	}
	return( this->at( key ) );
}

int Config::getInt( const std::string &key )
{
	try {
		std::istringstream ss( checkKey( key ) );
		int result;
		ss >> result;
		return result;
	}
	catch ( std::out_of_range& e ) {
		throw std::out_of_range("Key not found: " + key);
	}
}

unsigned long Config::getULong(const std::string& key)
{
    std::string value = checkKey(key);   // throws std::out_of_range if missing

    char* end = nullptr;
    errno = 0;

    unsigned long result = strtoul(value.c_str(), &end, 10);

    if (*end != '\0') {
        throw std::invalid_argument("Invalid unsigned long for key: " + key);
    }

    if (errno == ERANGE) {
        throw std::out_of_range("Unsigned long overflow for key: " + key);
    }

    return result;
}


double Config::getDouble( const std::string &key )
{
	try {
		std::istringstream ss( checkKey( key ) );
		double result;
		ss >> result;
		return result;
	}
	catch ( std::out_of_range& e ) {
		throw std::out_of_range("Key not found: " + key);

	}
}

std::string Config::getString( const std::string &key )
{
	std::istringstream ss( checkKey( key ) );
	std::string result;
	ss >> result;
	return result;
}

bool Config::getBool(const std::string& key) {
    std::istringstream ss(checkKey(key));
    std::string result;
    ss >> std::boolalpha >> result;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	if (result == "true" || result == "1") {
        return true;
    } else if (result == "false" || result == "0") {
        return false;
    } else {
        throw std::invalid_argument("Invalid boolean value: " + result + "for key= " + key);
    }
}


std::istream &operator >> ( std::istream &ins, Config &d )
{
	std::string s, key, value;

	ins.exceptions(std::ifstream::failbit);
	try {
		// For each (key, value) pair in the file
		while (std::getline( ins, s )) {
			std::string::size_type begin = s.find_first_not_of( " \f\t\v" );
			if (begin == std::string::npos) {
				continue;    // Skip blank lines
			}

			if (std::string( "#;" ).find( s[ begin ] ) != std::string::npos) {
				continue;    // Skip commentary
			}

			// Extract the key value
			std::string::size_type end = s.find( '=', begin );

			if(end == std::string::npos) continue;

			key = s.substr( begin, end - begin );

			key.erase( key.find_last_not_of( " \f\t\v" ) + 1 );  // (No leading or trailing whitespace allowed)

			if (key.empty()) { // No blank keys allowed
				continue;
			}

			// Extract the value (no leading or trailing whitespace allowed)
			begin = s.find_first_not_of( " \f\n\r\t\v", end + 1 );
			end   = s.find_last_not_of(  " \f\n\r\t\v" ) + 1;

			value = s.substr( begin, end - begin );

			// Insert the properly extracted (key, value) pair into the map
			d[key] = value;
		}
	}
	catch (const std::ifstream::failure &e) {
		// end of file
	}

	return ins;
}

std::shared_ptr<Config> Config::getInstance(const std::string& fn)
{
	static std::shared_ptr<Config> instance = std::make_shared<Config>(fn);
	return instance;
}

}