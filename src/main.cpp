
#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>
#include <thread>
#include <chrono>
#include <iostream>

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;


uint32_t linha = 0;
uint32_t coluna = 0;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
};

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

void entity_planta(){

    if(entity_grid[linha][coluna].age >= PLANT_MAXIMUM_AGE){
        entity_grid[linha][coluna] = {empty, 0, 0};
    } 
    else{
        entity_grid[linha][coluna].age += 1;
        if(rand()/double(RAND_MAX) < PLANT_REPRODUCTION_PROBABILITY){
            if(linha + 1 < NUM_ROWS && coluna + 1 < NUM_ROWS){
                if(entity_grid[linha+1][coluna].type == empty){
                    entity_grid[linha+1][coluna] = {plant, 0, 0};
                }
            }               
        }
    }
}



int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
        // Create the entities
        // <YOUR CODE HERE>
        int num_plants = (uint32_t)request_body["plants"];
        int num_herbivores = (uint32_t)request_body["herbivores"];
        int num_carnivores = (uint32_t)request_body["carnivores"];

        for (int i = 0; i < num_plants; i++)
        {
            pos_t pos = {rand() % NUM_ROWS, rand() % NUM_ROWS};
            entity_grid[pos.i][pos.j] = {plant, 0, 0};
        }

        for (int i = 0; i < num_herbivores; i++)
        {
            pos_t pos = {rand() % NUM_ROWS, rand() % NUM_ROWS};
            entity_grid[pos.i][pos.j] = {herbivore, 100, 0};
        }

        for (int i = 0; i < num_carnivores; i++)
        {
            pos_t pos = {rand() % NUM_ROWS, rand() % NUM_ROWS};
            entity_grid[pos.i][pos.j] = {carnivore, 0, 0};
        }

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behaviour of each entity
        
        // <YOUR CODE HERE>
        
        for (linha = 0; linha < NUM_ROWS; linha++)
        {
            for (coluna = 0; coluna < NUM_ROWS; coluna++)
            {
                if(entity_grid[linha][coluna].type == plant){
                    entity_planta();

                }
            }
        }
        

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}
