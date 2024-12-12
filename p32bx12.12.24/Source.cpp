#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <tgbot/tgbot.h>

const std::string TOKEN{ "7941234655:AAEfkLcHothQ14EpCUo9OGHNhx4Jdd8c22U" }; // Replace with your actual token


struct Pokemon {
    std::string name;
    int weight;
    int height;
    std::string photo;
};

nlohmann::json get_response(const std::string& pokemon_name) {
    cpr::Url url{ "https://pokeapi.co/api/v2/pokemon/" + pokemon_name };
    cpr::Response response = cpr::Get(url);

    if (response.status_code != 200) {
        throw std::runtime_error("Failed to fetch Pokémon data.");
    }

    return nlohmann::json::parse(response.text);
}

Pokemon get_pokemon(const nlohmann::json& json_pokemon) {
    Pokemon pokemon;
    pokemon.name = json_pokemon["name"];
    pokemon.weight = json_pokemon["weight"];
    pokemon.height = json_pokemon["height"];
    pokemon.photo = json_pokemon["sprites"]["other"]["official-artwork"]["front_default"];
    return pokemon;
}

std::string get_description(const Pokemon& pokemon) {
    std::string description;
    description += "Name: " + pokemon.name;
    description += "\nWeight: " + std::to_string(pokemon.weight);
    description += "\nHeight: " + std::to_string(pokemon.height);
    return description;
}

class PokemonList {
    std::vector<std::string> pokemon_list;
public:
    PokemonList() {
        cpr::Response response = cpr::Get(cpr::Url{ "https://pokeapi.co/api/v2/pokemon?limit=20&offset=0" });

        if (response.status_code != 200) {
            throw std::runtime_error("Failed to fetch Pokémon list.");
        }

        nlohmann::json json_response = nlohmann::json::parse(response.text);
        for (const auto& item : json_response["results"]) {
            pokemon_list.push_back(item["name"]);
        }
    }

    TgBot::InlineKeyboardMarkup::Ptr get_pokemon_keyboard() {
        auto keyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
        for (const auto& name : pokemon_list) {
            TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);
            button->text = name;
            button->callbackData = name; // Use Pokémon name as callback data
            keyboard->inlineKeyboard.push_back({ button });
        }
        return keyboard;
    }
};

int main() {
    TgBot::Bot bot(TOKEN);

    bot.getEvents().onCommand("menu", [&](TgBot::Message::Ptr message) {
        try {
            PokemonList pokemon_list;
            bot.getApi().sendMessage(message->chat->id, "Choose a Pokémon:", false, 0, pokemon_list.get_pokemon_keyboard());
        }
        catch (const std::exception& e) {
            bot.getApi().sendMessage(message->chat->id, "Error fetching Pokémon list: " + std::string(e.what()));
        }
        });

    bot.getEvents().onCallbackQuery([&](TgBot::CallbackQuery::Ptr query) {
        try {
            Pokemon pokemon = get_pokemon(get_response(query->data)); // Use callback data to get Pokémon name
            bot.getApi().sendPhoto(query->message->chat->id, pokemon.photo, get_description(pokemon));
        }
        catch (const std::exception& e) {
            bot.getApi().sendMessage(query->message->chat->id, "Error fetching Pokémon data: " + std::string(e.what()));
        }
        });

    try {
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    }
    catch (TgBot::TgException& e) {
        printf("Error: %s\n", e.what());
    }
}