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

#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <memory>

namespace extractor::util {

class Config: public std::map <std::string, std::string> {
private:	
	/**
	 * @brief isKey Is The configuration file contains the specified key value?
	 * @param key The key of the requested value
	 * @return TRUE if key exists otherwise FALSE
	 */
	bool isKey( const std::string &key ) const;
	std::string checkKey( const std::string &key ) const;

public:
	
	Config( const std::string& fn="" );

	/**
	 * @brief getEnvVariable Read the value of the environment variable
	 * @param key The environment variable
	 * @return Return value of the environment variable
	 */
	std::string getEnvVariable( const std::string &key ) const;

	/**
	 * @brief getInt read the integer value assigned to a key
	 * @param key The key which identify the configuration value
	 * @return The value if it was found otherwise throw an Out of range exception
	 */
	int getInt( const std::string &key );

	unsigned long getULong(const std::string& key);

	/**
	 * @brief getDouble read the double value assigned to a key
	 * @param key The key which identify the configuration value
	 * @return The value if it was found otherwise throw an Out of range exception
	 */
	double getDouble( const std::string &key );

	/**
	 * @brief getString read the string value assigned to a key
	 * @param key The key which identify the configuration value
	 */
	std::string getString( const std::string &key );

	/**
	 * @brief getBool read the bool value assigned to a key
	 * @param key The key which identify the configuration value
	 * @return The value if it was found otherwise throw an invalid argument exception
	 */
	bool getBool(const std::string& key);
	
	/**
	 * @brief operator >> Read in config file friend function
	 * @param ins  The stream the config read from.
	 * @param c	The Config object the file is read into.
	 * @return the stream
	 */
	friend std::istream &operator >> ( std::istream &ins, Config &c );
	static std::shared_ptr<Config> getInstance(const std::string& fn="");

};


}	// namespace

#endif // CONFIG_H