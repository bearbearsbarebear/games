#include "ConfigManager.h"

boost::property_tree::ptree ConfigManager::pt;
int32_t ConfigManager::highHealth;
int32_t ConfigManager::lowHealth;
int32_t ConfigManager::mana;
int32_t ConfigManager::healParalyze;
std::string ConfigManager::highHealthkey;
std::string ConfigManager::lowHealthSpellKey;
std::string ConfigManager::lowHealthItemKey;
std::string ConfigManager::manaKey;
std::string ConfigManager::healParalyzeKey;

int32_t ConfigManager::loadConfig()
{
    try {
        boost::property_tree::ini_parser::read_ini("config.ini", ConfigManager::pt);
    } catch (const boost::property_tree::ini_parser_error& e) {
        return 1;
    } catch (const std::exception& e) {
        return 2;
    }

    // Retrieve and store the values
    try {
        ConfigManager::highHealth = ConfigManager::pt.get<int>("HighHealth");
        ConfigManager::lowHealth = ConfigManager::pt.get<int>("LowHealth");
        ConfigManager::mana = ConfigManager::pt.get<int>("Mana");

        ConfigManager::healParalyze = ConfigManager::pt.get<int>("HealParalyze");

        ConfigManager::highHealthkey = ConfigManager::pt.get<std::string>("HighHealthKey");
        ConfigManager::lowHealthSpellKey = ConfigManager::pt.get<std::string>("LowHealthSpellKey");
        ConfigManager::lowHealthItemKey = ConfigManager::pt.get<std::string>("LowHealthItemKey");
        ConfigManager::manaKey = ConfigManager::pt.get<std::string>("ManaKey");
        ConfigManager::healParalyzeKey = ConfigManager::pt.get<std::string>("HealParalyzeKey");
    } catch (const boost::property_tree::ptree_error& e) {
        return 3;
    }

    return 0;
}

int32_t ConfigManager::stringToKeyCode(std::string keyString)
{
    // Remove the "0x" prefix from the string
    if (keyString.substr(0, 2) == "0x") keyString = keyString.substr(2);

    // Convert the hexadecimal string to an integer
    int keyValue = std::stoi(keyString, nullptr, 16);

    return keyValue;
}
