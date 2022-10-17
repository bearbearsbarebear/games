#include <iostream>
#include <time.h>

#include <dpp/dpp.h>

struct Mission
{
	long TimeLeft;
	char* PlaceName;
	char* Description;
};

std::vector<dpp::message> messagesStack;
bool isSugarActive = false;
dpp::snowflake channel_id;
std::vector<dpp::snowflake> role = { 1007398261465813013 };

/*
* Curent version is working fine
* TODO:
* Make role mention work somehow
* Refactor everything with classes
* Debug some important info while it runs
*/

// Function from https://github.com/Lohkdesgds/Bread
void deleteSlashCommands(dpp::cluster& bot)
{
	bot.global_commands_get([&bot](const dpp::confirmation_callback_t& dat) {
		if (dat.is_error()) {
			std::cout << "Error getting global commands for deletion: " << dat.get_error().message << std::endl;
			std::cout << "Full HTTP: \n" << dat.http_info.body << std::endl;
			return;
		}

		dpp::slashcommand_map mappo = std::get<dpp::slashcommand_map>(dat.value);

		for (auto& it : mappo) {
			bot.global_command_delete(it.first, [secnam = it.second.name](const dpp::confirmation_callback_t& dat){
				if (dat.is_error()) {
					std::cout << "Error deleting global command: " << secnam << std::endl;
				}
				else {
					std::cout << "Removed global command: " << secnam << std::endl;
				}
			});
		}

		std::cout << "Queued all global commands deletion." << std::endl;
	});
}

void printSlashCommands(dpp::cluster& bot)
{
	bot.global_commands_get([&bot](const dpp::confirmation_callback_t& dat) {
		if (dat.is_error()) {
			std::cout << "Error getting global commands: " << dat.get_error().message << std::endl;
			std::cout << "Full HTTP: \n" << dat.http_info.body << std::endl;
			return;
		}

		dpp::slashcommand_map mappo = std::get<dpp::slashcommand_map>(dat.value);

		for (auto& it : mappo) {
			std::cout << it.second.name << std::endl;
		}

		std::cout << std::endl;
	});
}

void deleteMessagesStack(dpp::cluster& bot)
{
	for (std::vector<dpp::message>::iterator m = messagesStack.begin(); m != messagesStack.end();) {
		bot.message_delete(m->id, m->channel_id);
		m = messagesStack.erase(m);
	}
}

void setupOnReady(dpp::cluster& bot, const dpp::ready_t& event)
{
	// Not exactly good practice registering commands inside onReady, but wellp
	if (dpp::run_once<struct register_bot_commands>()) {
		/* Create new global commands on ready event */
		dpp::slashcommand _start("start", "(Admin) Starts event fetching", bot.me.id);
		dpp::slashcommand _stop("stop", "(Admin) Stops event fetching", bot.me.id);

		/* Register the commands */
		bot.global_command_create(_start);
		bot.global_command_create(_stop);
	}
}

void setupSlashCommand(dpp::cluster& bot, const dpp::slashcommand_t& event)
{
	/* Check which command was ran */
	if (event.command.get_command_name() == "start") {
		event.reply(std::string("Starting event fetching~"));
		channel_id = event.command.channel_id;
		isSugarActive = true;
	}
	else if (event.command.get_command_name() == "stop") {
		event.reply(std::string("Stopping my services..."));
		isSugarActive = false;
	}
}

void setupOnMessageCreate(dpp::cluster& bot, const dpp::message_create_t& event)
{
	// Pushes every Sugar's messages to the messages stack
	if (bot.me.id == event.msg.author.id) { 
		dpp::message* m = new dpp::message();
		*m = event.msg;
		messagesStack.push_back(*m);
	}
}

void setupBot(dpp::cluster& bot)
{
	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t& event) { setupOnReady(bot, event); });
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) { setupSlashCommand(bot, event); });
	bot.on_message_create([&](const dpp::message_create_t& event) { setupOnMessageCreate(bot, event); });
}

void eraseMissionList(std::vector<Mission*>* missionList)
{
	for (std::vector<Mission*>::iterator m = missionList->begin(); m != missionList->end();)
	{
		delete[](*m)->Description;
		delete[](*m)->PlaceName;

		Mission* prevMission = *m;

		m = missionList->erase(m);

		delete prevMission;
	}
}

void copyToMissionList(std::vector<Mission*>* missionList)
{
	std::ifstream ifstream("C:\\Log.txt");
	std::string line;

	while (getline(ifstream, line)) {
		if (line == "") continue;
		Mission* mission = new Mission();

		// First line is PlaceName
		mission->PlaceName = new char[line.length() + 1];
		strcpy_s(mission->PlaceName, line.length() + 1, line.c_str());

		// Second Line is TimeLeft
		getline(ifstream, line);
		mission->TimeLeft = std::stoi(line) + time(NULL);

		// Third line is Description
		getline(ifstream, line);

		// Check if the retrieved line is a Wanted Pirate, in case it's not
		// It should free PlaceName and mission and break 
		if (line.find("spotted") == std::string::npos) {
			delete[] mission->PlaceName;
			delete[] mission;
			break;
		}

		mission->Description = new char[line.length() + 1];
		strcpy_s(mission->Description, line.length() + 1, line.c_str());

		missionList->push_back(mission);
	}

	ifstream.close();
}

void printMissionlist(std::vector<Mission*>* missionList)
{
	for (Mission* m : *missionList) {
		std::cout << m->PlaceName << std::endl;
		std::cout << m->TimeLeft - time(NULL) << std::endl;
		std::cout << m->Description << std::endl << std::endl;
	}
}

bool compareLists(std::vector<Mission*>* missionList, std::vector<Mission*>* tempList)
{
	if (missionList->size() == 0 || tempList->size() == 0) return false;

	for (Mission* m : *missionList) {
		for (Mission* t : *tempList) {
			if (strcmp(m->Description, t->Description) == 0) return true;
		}
	}

	return false;
}

void postMissionlist(dpp::cluster& bot, std::vector<Mission*>* missionList)
{
	dpp::embed embed = dpp::embed().
		set_color(dpp::colors::sti_blue).
		set_title("Wanted Pirates").
		set_description("Events happening right now:").
		set_thumbnail("https://cdn.discordapp.com/attachments/1004806114552070191/1007390724360257596/unknown.png");

	for (Mission* m : *missionList) {
		embed.add_field(
			m->PlaceName,
			m->Description
		);
	}

	embed.set_footer(dpp::embed_footer().set_text("owo").
		set_icon("https://cdn.discordapp.com/attachments/1004806114552070191/1007390724360257596/unknown.png")).
		set_timestamp(time(0));

	dpp::message everyone(channel_id, embed);
	everyone.channel_id = channel_id;
	everyone.content = "OwO";
	everyone.mention_roles = role;
	bot.message_create(everyone);
}

void mainLoop(dpp::cluster& bot, std::vector<Mission*>* missionList, std::vector<Mission*>* tempList)
{
	if (!isSugarActive) return;

	// Get current list to temp list
	eraseMissionList(tempList);
	copyToMissionList(tempList);

	// If both are equal, no point to do anything
	if (compareLists(missionList, tempList)) return;

	// New lists, gotta update missionList and post them
	deleteMessagesStack(bot);

	eraseMissionList(missionList);
	copyToMissionList(missionList);
	postMissionlist(bot, missionList);
}

int main(int argc, char* agv[])
{
	std::vector<Mission*> missionList;
	std::vector<Mission*> tempList;

	dpp::cluster bot("BOT_TOKEN", dpp::i_default_intents | dpp::i_message_content);

	setupBot(bot);

	auto task_looper = bot.start_timer([&](auto) { mainLoop(bot, &missionList, &tempList); }, 30);

	bot.start(dpp::st_wait);
	
	// Exiting
	bot.stop_timer(task_looper);
	std::cout << "Sugar stopped successfully." << std::endl;

	return 0;
}
