
#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <ctime>


static const uint32_t NUM_ROWS = 15;

std::mutex mtx;
std::condition_variable cv_thread;
bool thread_ativa = false;


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

std::vector<pos_t> posicoes;

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

bool random_action(float probability) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

bool verifica_posicao(uint32_t i, uint32_t j){
    pos_t pos_atual = {i,j};
    bool analisado = false;
    for(uint32_t k = 0; k < posicoes.size(); k++)
    {
        if(posicoes[k].i == i && posicoes[k].j == j){
            analisado = true;
        }
    }
    return analisado;
}

void cresc_planta(uint32_t i, uint32_t j){
    std::vector<int> vec_cresc;
    if(i + 1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == empty){
        vec_cresc.push_back(1);
        }
    }
    if(i > 0) {
        if(entity_grid[i-1][j].type == empty){
            vec_cresc.push_back(2);
        }
    }
    if(j + 1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == empty){
            vec_cresc.push_back(3);
        }
    }
    if(j > 0) {
        if(entity_grid[i][j-1].type == empty){
            vec_cresc.push_back(4);
        }
    }

    unsigned seed = time(0);
    srand(seed);

    int caso = 0;
    if(vec_cresc.size() > 0){
        int pos_vect = rand() % vec_cresc.size();
        caso = vec_cresc[pos_vect];
    }

    pos_t pos;
    
    switch (caso)
    {
    case 1:
        entity_grid[i+1][j] = {plant, 0, 0};
        pos = {i+1,j};
        posicoes.push_back(pos);
        break;
    case 2:
        entity_grid[i-1][j] = {plant, 0, 0};
        pos = {i-1,j};
        posicoes.push_back(pos);
        break;
    case 3:
        entity_grid[i][j+1] = {plant, 0, 0};
        pos = {i,j+1};
        posicoes.push_back(pos);
        break;
    case 4:
        entity_grid[i][j-1] = {plant, 0, 0};
        pos = {i,j-1};
        posicoes.push_back(pos);
        break;
    }
}

void entity_planta(uint32_t i, uint32_t j){
    std::unique_lock lk(mtx);

    while(!thread_ativa){
        cv_thread.wait(lk);
    }

    if(entity_grid[i][j].age >= PLANT_MAXIMUM_AGE){
        entity_grid[i][j] = {empty, 0, 0};
    } 
    else {
        entity_grid[i][j].age += 1;
        if(random_action(PLANT_REPRODUCTION_PROBABILITY)){
            cresc_planta(i, j);             
        }
    } 
}


void repr_herbivoro(uint32_t i, uint32_t j){
    std::vector<int> vec_repr;
    if(i + 1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == empty){
        vec_repr.push_back(1);
        }
    }
    if(i > 0) {
        if(entity_grid[i-1][j].type == empty){
            vec_repr.push_back(2);
        }
    }
    if(j + 1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == empty){
            vec_repr.push_back(3);
        }
    }
    if(j > 0) {
        if(entity_grid[i][j-1].type == empty){
            vec_repr.push_back(4);
        }
    }

    unsigned seed = time(0);
    srand(seed);

    int caso = 0;
    if(vec_repr.size() > 0){
        int pos_vect = rand() % vec_repr.size();
        caso = vec_repr[pos_vect];
    }

    pos_t pos;
    
    switch (caso)
    {
    case 1:
        entity_grid[i+1][j] = {herbivore, 100, 0};
        entity_grid[i][j].energy -= 10;
        pos = {i+1,j};
        posicoes.push_back(pos);
        break;
    case 2:
        entity_grid[i-1][j] = {herbivore, 100, 0};
        entity_grid[i][j].energy -= 10;
        pos = {i-1,j};
        posicoes.push_back(pos);
        break;
    case 3:
        entity_grid[i][j+1] = {herbivore, 100, 0};
        entity_grid[i][j].energy -= 10;
        pos = {i,j+1};
        posicoes.push_back(pos);
        break;
    case 4:
        entity_grid[i][j-1] = {herbivore, 100, 0};
        entity_grid[i][j].energy -= 10;
        pos = {i,j-1};
        posicoes.push_back(pos);
        break;
    }
}

void mov_herbivoro(uint32_t i, uint32_t j){
    std::vector<int> vec_mov;
    entity_t herbivoro = entity_grid[i][j];
    if(i + 1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == empty){
        vec_mov.push_back(1);
        }
    }
    if(i > 0) {
        if(entity_grid[i-1][j].type == empty){
            vec_mov.push_back(2);
        }
    }
    if(j + 1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == empty){
            vec_mov.push_back(3);
        }
    }
    if(j > 0) {
        if(entity_grid[i][j-1].type == empty){
            vec_mov.push_back(4);
        }
    }

    unsigned seed = time(0);
    srand(seed);

    int caso = 0;
    if(vec_mov.size() > 0){
        int pos_vect = rand() % vec_mov.size();
        caso = vec_mov[pos_vect];
    }

    pos_t pos;

    switch (caso)
    {
    case 1:
        entity_grid[i][j] = {empty, 0, 0};
        entity_grid[i+1][j] = herbivoro;
        entity_grid[i+1][j].energy -= 5;
        pos = {i+1,j};
        posicoes.push_back(pos);
        break;
    case 2:
        entity_grid[i][j] = {empty, 0, 0};
        entity_grid[i-1][j] = herbivoro;
        entity_grid[i-1][j].energy -= 5;
        pos = {i-1,j};
        posicoes.push_back(pos);
        break;
    case 3:
        entity_grid[i][j] = {empty, 0, 0};
        entity_grid[i][j+1] = herbivoro;
        entity_grid[i][j+1].energy -= 5;
        pos = {i,j+1};
        posicoes.push_back(pos);
        break;
    case 4:
        entity_grid[i][j] = {empty, 0, 0};
        entity_grid[i][j-1] = herbivoro;
        entity_grid[i][j-1].energy -= 5;
        pos = {i,j-1};
        posicoes.push_back(pos);
        break;
    }
}

void come_herbivoro(uint32_t i, uint32_t j){
    std::vector<int> vec_come;
    if(i + 1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == plant){
        vec_come.push_back(1);
        }
    }
    if(i > 0) {
        if(entity_grid[i-1][j].type == plant){
            vec_come.push_back(2);
        }
    }
    if(j + 1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == plant){
            vec_come.push_back(3);
        }
    }
    if(j > 0) {
        if(entity_grid[i][j-1].type == plant){
            vec_come.push_back(4);
        }
    }

    unsigned seed = time(0);
    srand(seed);

    int caso = 0;
    if(vec_come.size() > 0){
        int pos_vect = rand() % vec_come.size();
        caso = vec_come[pos_vect];
    }

    switch (caso)
    {
    case 1:
        entity_grid[i+1][j] = {empty, 0, 0};
        entity_grid[i][j].energy += 30;
        break;
    case 2:
        entity_grid[i-1][j] = {empty, 0, 0};
        entity_grid[i][j].energy += 30;
        break;
    case 3:
        entity_grid[i][j+1] = {empty, 0, 0};
        entity_grid[i][j].energy += 30;
        break;
    case 4:
        entity_grid[i][j-1] = {empty, 0, 0};
        entity_grid[i][j].energy += 30;
        break;
    }
}

void entity_herbivoro(uint32_t i, uint32_t j){
    std::unique_lock lk(mtx);

    while(!thread_ativa){
        cv_thread.wait(lk);
    }

    if(entity_grid[i][j].age >= HERBIVORE_MAXIMUM_AGE || entity_grid[i][j].energy == 0){
        entity_grid[i][j] = {empty, 0, 0};
    } 
    else {
        entity_grid[i][j].age += 1;
        if(random_action(HERBIVORE_EAT_PROBABILITY)){
            come_herbivoro(i, j);
        }
        if(random_action(HERBIVORE_REPRODUCTION_PROBABILITY) && entity_grid[i][j].energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION){
            repr_herbivoro(i, j);             
        }
        if(random_action(HERBIVORE_MOVE_PROBABILITY) && entity_grid[i][j].energy >= 5){
            mov_herbivoro(i, j);
        } 
    }
}


void repr_carnivoro(uint32_t i, uint32_t j){
    std::vector<int> vec_repr;
    if(i + 1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == empty){
        vec_repr.push_back(1);
        }
    }
    if(i > 0) {
        if(entity_grid[i-1][j].type == empty){
            vec_repr.push_back(2);
        }
    }
    if(j + 1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == empty){
            vec_repr.push_back(3);
        }
    }
    if(j > 0) {
        if(entity_grid[i][j-1].type == empty){
            vec_repr.push_back(4);
        }
    }

    unsigned seed = time(0);
    srand(seed);

    int caso = 0;
    if(vec_repr.size() > 0){
        int pos_vect = rand() % vec_repr.size();
        caso = vec_repr[pos_vect];
        
    }
    
    pos_t pos;

    switch (caso)
    {
    case 1:
        entity_grid[i+1][j] = {carnivore, 100, 0};
        entity_grid[i][j].energy -= 10;
        pos = {i+1,j};
        posicoes.push_back(pos);
        break;
    case 2:
        entity_grid[i-1][j] = {carnivore, 100, 0};
        entity_grid[i][j].energy -= 10;
        pos = {i-1,j};
        posicoes.push_back(pos);
        break;
    case 3:
        entity_grid[i][j+1] = {carnivore, 100, 0};
        entity_grid[i][j].energy -= 10;
        pos = {i,j+1};
        posicoes.push_back(pos);
        break;
    case 4:
        entity_grid[i][j-1] = {carnivore, 100, 0};
        entity_grid[i][j].energy -= 10;
        pos = {i,j-1};
        posicoes.push_back(pos);
        break;
    }
}

void mov_carnivoro(uint32_t i, uint32_t j){
    std::vector<int> vec_mov;
    entity_t carnivoro = entity_grid[i][j];
    if(i + 1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == empty || entity_grid[i+1][j].type == herbivore){
        vec_mov.push_back(1);
        }
    }
    if(i > 0) {
        if(entity_grid[i-1][j].type == empty || entity_grid[i-1][j].type == herbivore){
            vec_mov.push_back(2);
        }
    }
    if(j + 1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == empty || entity_grid[i][j+1].type == herbivore){
            vec_mov.push_back(3);
        }
    }
    if(j > 0) {
        if(entity_grid[i][j-1].type == empty || entity_grid[i][j-1].type == herbivore){
            vec_mov.push_back(4);
        }
    }

    unsigned seed = time(0);
    srand(seed);

    int caso = 0;
    if(vec_mov.size() > 0){
        int pos_vect = rand() % vec_mov.size();
        caso = vec_mov[pos_vect];
    }

    pos_t pos;

    switch (caso)
    {
    case 1:
        entity_grid[i][j] = {empty, 0, 0};
        entity_grid[i+1][j] = carnivoro;
        entity_grid[i+1][j].energy -= 5;
        pos = {i+1,j};
        posicoes.push_back(pos);
        break;
    case 2:
        entity_grid[i][j] = {empty, 0, 0};
        entity_grid[i-1][j] = carnivoro;
        entity_grid[i-1][j].energy -= 5;
        pos = {i-1,j};
        posicoes.push_back(pos);
        break;
    case 3:
        entity_grid[i][j] = {empty, 0, 0};
        entity_grid[i][j+1] = carnivoro;
        entity_grid[i][j+1].energy -= 5;
        pos = {i,j+1};
        posicoes.push_back(pos);
        break;
    case 4:
        entity_grid[i][j] = {empty, 0, 0};
        entity_grid[i][j-1] = carnivoro;
        entity_grid[i][j-1].energy -= 5;
        pos = {i,j-1};
        posicoes.push_back(pos);
        break;
    }
}


void come_carnivoro(uint32_t i, uint32_t j){
    std::vector<int> vec_come;
    if(i + 1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == herbivore){
        vec_come.push_back(1);
        }
    }
    if(i > 0) {
        if(entity_grid[i-1][j].type == herbivore){
            vec_come.push_back(2);
        }
    }
    if(j + 1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == herbivore){
            vec_come.push_back(3);
        }
    }
    if(j > 0) {
        if(entity_grid[i][j-1].type == herbivore){
            vec_come.push_back(4);
        }
    }

    unsigned seed = time(0);
    srand(seed);

    int caso = 0;
    if(vec_come.size() > 0){
        int pos_vect = rand() % vec_come.size();
        caso = vec_come[pos_vect];
    }

    switch (caso)
    {
    case 1:
        entity_grid[i+1][j] = {empty, 0, 0};
        entity_grid[i][j].energy += 20;
        break;
    case 2:
        entity_grid[i-1][j] = {empty, 0, 0};
        entity_grid[i][j].energy += 20;
        break;
    case 3:
        entity_grid[i][j+1] = {empty, 0, 0};
        entity_grid[i][j].energy += 20;
        break;
    case 4:
        entity_grid[i][j-1] = {empty, 0, 0};
        entity_grid[i][j].energy += 20;
        break;
    }
}

void entity_carnivoro(uint32_t i, uint32_t j){
    std::unique_lock lk(mtx);

    while(!thread_ativa){
        cv_thread.wait(lk);
    }

    if(entity_grid[i][j].age >= CARNIVORE_MAXIMUM_AGE || entity_grid[i][j].energy == 0){
        entity_grid[i][j] = {empty, 0, 0};
    } 
    else {
        entity_grid[i][j].age += 1;
        come_carnivoro(i, j);
        if(random_action(CARNIVORE_REPRODUCTION_PROBABILITY) && entity_grid[i][j].energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION){
            repr_carnivoro(i, j);             
        }
        if(random_action(CARNIVORE_MOVE_PROBABILITY) && entity_grid[i][j].energy >= 5){
            mov_carnivoro(i, j);
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

        unsigned seed = time(0);
        srand(seed);

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
            entity_grid[pos.i][pos.j] = {carnivore, 100, 0};
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
        std::vector<std::thread> threads;

        mtx.lock();

        for (uint32_t i = 0; i < NUM_ROWS; i++)
        {
            for (uint32_t j = 0; j < NUM_ROWS; j++)
            {
                if(entity_grid[i][j].type == plant){
                    threads.push_back(std::thread(entity_planta, i, j));
                }
                if(entity_grid[i][j].type == herbivore){
                    threads.push_back(std::thread(entity_herbivoro, i, j));
                }
                if(entity_grid[i][j].type == carnivore){
                    threads.push_back(std::thread(entity_carnivoro, i, j));
                    
                }
            }
        }

        thread_ativa = true;
        mtx.unlock();
        cv_thread.notify_all();
        
        for (int i=0; i< threads.size(); ++i)
	    {
	    	threads[i].join();
	    }
        
        thread_ativa = false;

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}