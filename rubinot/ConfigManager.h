#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <stdexcept>

class ConfigManager 
{
public:
    static int32_t loadConfig();
    static int32_t stringToKeyCode(std::string keyString);

    static boost::property_tree::ptree pt;
    static int32_t highHealth;
    static int32_t lowHealth;
    static int32_t mana;
    static int32_t healParalyze;
    static std::string highHealthkey;
    static std::string lowHealthSpellKey;
    static std::string lowHealthItemKey;
    static std::string manaKey;
    static std::string healParalyzeKey;
};
