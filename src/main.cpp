
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

int counter;


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

bool verifica_posicao(){
    pos_t pos_atual = {linha,coluna};
    bool analisado = false;
    for(int i = 0; i < posicoes.size(); i++)
    {
        if(posicoes[i].i == linha && posicoes[i].j == coluna){
            analisado = true;
        }
    }
    return analisado;
}

void cresc_planta(){
    std::vector<int> vec_cresc;
    if(linha + 1 < NUM_ROWS) {
        if(entity_grid[linha+1][coluna].type == empty){
        vec_cresc.push_back(1);
        }
    }
    if(linha > 0) {
        if(entity_grid[linha-1][coluna].type == empty){
            vec_cresc.push_back(2);
        }
    }
    if(coluna + 1 < NUM_ROWS) {
        if(entity_grid[linha][coluna+1].type == empty){
            vec_cresc.push_back(3);
        }
    }
    if(coluna > 0) {
        if(entity_grid[linha][coluna-1].type == empty){
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
    
    switch (caso)
    {
    case 1:
        entity_grid[linha+1][coluna] = {plant, 0, 0};
        break;
    case 2:
        entity_grid[linha-1][coluna] = {plant, 0, 0};
        break;
    case 3:
        entity_grid[linha][coluna+1] = {plant, 0, 0};
        break;
    case 4:
        entity_grid[linha][coluna-1] = {plant, 0, 0};
        break;
    }
}

void entity_planta(){
    //while(true){
        mtx.lock();
        if(entity_grid[linha][coluna].age >= PLANT_MAXIMUM_AGE){
        entity_grid[linha][coluna] = {empty, 0, 0};
        //break;
    } 
    else {
        entity_grid[linha][coluna].age += 1;
        if(random_action(PLANT_REPRODUCTION_PROBABILITY)){
            cresc_planta();             
        }
    }
    //}
    mtx.unlock();
    
}


void repr_herbivoro(){
    std::vector<int> vec_repr;
    if(linha + 1 < NUM_ROWS) {
        if(entity_grid[linha+1][coluna].type == empty){
        vec_repr.push_back(1);
        }
    }
    if(linha > 0) {
        if(entity_grid[linha-1][coluna].type == empty){
            vec_repr.push_back(2);
        }
    }
    if(coluna + 1 < NUM_ROWS) {
        if(entity_grid[linha][coluna+1].type == empty){
            vec_repr.push_back(3);
        }
    }
    if(coluna > 0) {
        if(entity_grid[linha][coluna-1].type == empty){
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
    
    switch (caso)
    {
    case 1:
        entity_grid[linha+1][coluna] = {herbivore, 100, 0};
        entity_grid[linha][coluna].energy -= 10;
        break;
    case 2:
        entity_grid[linha-1][coluna] = {herbivore, 100, 0};
        entity_grid[linha][coluna].energy -= 10;
        break;
    case 3:
        entity_grid[linha][coluna+1] = {herbivore, 100, 0};
        entity_grid[linha][coluna].energy -= 10;
        break;
    case 4:
        entity_grid[linha][coluna-1] = {herbivore, 100, 0};
        entity_grid[linha][coluna].energy -= 10;
        break;
    }
}

void mov_herbivoro(){
    std::vector<int> vec_mov;
    entity_t herbivoro = entity_grid[linha][coluna];
    if(linha + 1 < NUM_ROWS) {
        if(entity_grid[linha+1][coluna].type == empty){
        vec_mov.push_back(1);
        }
    }
    if(linha > 0) {
        if(entity_grid[linha-1][coluna].type == empty){
            vec_mov.push_back(2);
        }
    }
    if(coluna + 1 < NUM_ROWS) {
        if(entity_grid[linha][coluna+1].type == empty){
            vec_mov.push_back(3);
        }
    }
    if(coluna > 0) {
        if(entity_grid[linha][coluna-1].type == empty){
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
        entity_grid[linha][coluna] = {empty, 0, 0};
        entity_grid[linha+1][coluna] = herbivoro;
        entity_grid[linha+1][coluna].energy -= 5;
        pos = {linha+1,coluna};
        posicoes.push_back(pos);
        break;
    case 2:
        entity_grid[linha][coluna] = {empty, 0, 0};
        entity_grid[linha-1][coluna] = herbivoro;
        entity_grid[linha-1][coluna].energy -= 5;
        pos = {linha-1,coluna};
        posicoes.push_back(pos);
        break;
    case 3:
        entity_grid[linha][coluna] = {empty, 0, 0};
        entity_grid[linha][coluna+1] = herbivoro;
        entity_grid[linha][coluna+1].energy -= 5;
        pos = {linha,coluna+1};
        posicoes.push_back(pos);
        break;
    case 4:
        entity_grid[linha][coluna] = {empty, 0, 0};
        entity_grid[linha][coluna-1] = herbivoro;
        entity_grid[linha][coluna-1].energy -= 5;
        pos = {linha,coluna-1};
        posicoes.push_back(pos);
        break;
    }
}

void come_herbivoro(){
    std::vector<int> vec_come;
    if(linha + 1 < NUM_ROWS) {
        if(entity_grid[linha+1][coluna].type == plant){
        vec_come.push_back(1);
        }
    }
    if(linha > 0) {
        if(entity_grid[linha-1][coluna].type == plant){
            vec_come.push_back(2);
        }
    }
    if(coluna + 1 < NUM_ROWS) {
        if(entity_grid[linha][coluna+1].type == plant){
            vec_come.push_back(3);
        }
    }
    if(coluna > 0) {
        if(entity_grid[linha][coluna-1].type == plant){
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
        entity_grid[linha+1][coluna] = {empty, 0, 0};
        entity_grid[linha][coluna].energy += 30;
        break;
    case 2:
        entity_grid[linha-1][coluna] = {empty, 0, 0};
        entity_grid[linha][coluna].energy += 30;
        break;
    case 3:
        entity_grid[linha][coluna+1] = {empty, 0, 0};
        entity_grid[linha][coluna].energy += 30;
        break;
    case 4:
        entity_grid[linha][coluna-1] = {empty, 0, 0};
        entity_grid[linha][coluna].energy += 30;
        break;
    }
}

void entity_herbivoro(){
    mtx.lock();
    if(entity_grid[linha][coluna].age >= HERBIVORE_MAXIMUM_AGE || entity_grid[linha][coluna].energy == 0){
        entity_grid[linha][coluna] = {empty, 0, 0};
    } 
    else {
        if(!verifica_posicao()){
            entity_grid[linha][coluna].age += 1;
            if(random_action(HERBIVORE_EAT_PROBABILITY)){
                come_herbivoro();
            }
            if(random_action(HERBIVORE_REPRODUCTION_PROBABILITY) && entity_grid[linha][coluna].energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION){
                repr_herbivoro();             
            }
            if(random_action(HERBIVORE_MOVE_PROBABILITY) && entity_grid[linha][coluna].energy >= 5){
                mov_herbivoro();
            }
        }
        
    }
    mtx.unlock();
}


void repr_carnivoro(){
    std::vector<int> vec_repr;
    if(linha + 1 < NUM_ROWS) {
        if(entity_grid[linha+1][coluna].type == empty){
        vec_repr.push_back(1);
        }
    }
    if(linha > 0) {
        if(entity_grid[linha-1][coluna].type == empty){
            vec_repr.push_back(2);
        }
    }
    if(coluna + 1 < NUM_ROWS) {
        if(entity_grid[linha][coluna+1].type == empty){
            vec_repr.push_back(3);
        }
    }
    if(coluna > 0) {
        if(entity_grid[linha][coluna-1].type == empty){
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
        entity_grid[linha+1][coluna] = {carnivore, 100, 0};
        entity_grid[linha][coluna].energy -= 10;
        pos = {linha+1,coluna};
        posicoes.push_back(pos);
        break;
    case 2:
        entity_grid[linha-1][coluna] = {carnivore, 100, 0};
        entity_grid[linha][coluna].energy -= 10;
        pos = {linha-1,coluna};
        posicoes.push_back(pos);
        break;
    case 3:
        entity_grid[linha][coluna+1] = {carnivore, 100, 0};
        entity_grid[linha][coluna].energy -= 10;
        pos = {linha,coluna+1};
        posicoes.push_back(pos);
        break;
    case 4:
        entity_grid[linha][coluna-1] = {carnivore, 100, 0};
        entity_grid[linha][coluna].energy -= 10;
        pos = {linha,coluna-1};
        posicoes.push_back(pos);
        break;
    }
}

void mov_carnivoro(){
    std::vector<int> vec_mov;
    entity_t carnivoro = entity_grid[linha][coluna];
    if(linha + 1 < NUM_ROWS) {
        if(entity_grid[linha+1][coluna].type == empty || entity_grid[linha+1][coluna].type == herbivore){
        vec_mov.push_back(1);
        }
    }
    if(linha > 0) {
        if(entity_grid[linha-1][coluna].type == empty || entity_grid[linha-1][coluna].type == herbivore){
            vec_mov.push_back(2);
        }
    }
    if(coluna + 1 < NUM_ROWS) {
        if(entity_grid[linha][coluna+1].type == empty || entity_grid[linha][coluna+1].type == herbivore){
            vec_mov.push_back(3);
        }
    }
    if(coluna > 0) {
        if(entity_grid[linha][coluna-1].type == empty || entity_grid[linha][coluna-1].type == herbivore){
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
        entity_grid[linha][coluna] = {empty, 0, 0};
        entity_grid[linha+1][coluna] = carnivoro;
        entity_grid[linha+1][coluna].energy -= 5;
        pos = {linha+1,coluna};
        posicoes.push_back(pos);
        break;
    case 2:
        entity_grid[linha][coluna] = {empty, 0, 0};
        entity_grid[linha-1][coluna] = carnivoro;
        entity_grid[linha-1][coluna].energy -= 5;
        pos = {linha-1,coluna};
        posicoes.push_back(pos);
        break;
    case 3:
        entity_grid[linha][coluna] = {empty, 0, 0};
        entity_grid[linha][coluna+1] = carnivoro;
        entity_grid[linha][coluna+1].energy -= 5;
        pos = {linha,coluna+1};
        posicoes.push_back(pos);
        break;
    case 4:
        entity_grid[linha][coluna] = {empty, 0, 0};
        entity_grid[linha][coluna-1] = carnivoro;
        entity_grid[linha][coluna-1].energy -= 5;
        pos = {linha,coluna-1};
        posicoes.push_back(pos);
        break;
    }
}


void come_carnivoro(){
    std::vector<int> vec_come;
    if(linha + 1 < NUM_ROWS) {
        if(entity_grid[linha+1][coluna].type == herbivore){
        vec_come.push_back(1);
        }
    }
    if(linha > 0) {
        if(entity_grid[linha-1][coluna].type == herbivore){
            vec_come.push_back(2);
        }
    }
    if(coluna + 1 < NUM_ROWS) {
        if(entity_grid[linha][coluna+1].type == herbivore){
            vec_come.push_back(3);
        }
    }
    if(coluna > 0) {
        if(entity_grid[linha][coluna-1].type == herbivore){
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
        entity_grid[linha+1][coluna] = {empty, 0, 0};
        entity_grid[linha][coluna].energy += 20;
        break;
    case 2:
        entity_grid[linha-1][coluna] = {empty, 0, 0};
        entity_grid[linha][coluna].energy += 20;
        break;
    case 3:
        entity_grid[linha][coluna+1] = {empty, 0, 0};
        entity_grid[linha][coluna].energy += 20;
        break;
    case 4:
        entity_grid[linha][coluna-1] = {empty, 0, 0};
        entity_grid[linha][coluna].energy += 20;
        break;
    }
}

void entity_carnivoro(){
    mtx.lock();
    if(entity_grid[linha][coluna].age >= CARNIVORE_MAXIMUM_AGE || entity_grid[linha][coluna].energy == 0){
        entity_grid[linha][coluna] = {empty, 0, 0};
    } 
    else {
        if(!verifica_posicao()){
            entity_grid[linha][coluna].age += 1;
            come_carnivoro();
            if(random_action(CARNIVORE_REPRODUCTION_PROBABILITY) && entity_grid[linha][coluna].energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION){
                repr_carnivoro();             
            }
            if(random_action(CARNIVORE_MOVE_PROBABILITY) && entity_grid[linha][coluna].energy >= 5){
                mov_carnivoro();
            }
        }
        
    }
   mtx.unlock();
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

        

        for (linha = 0; linha < NUM_ROWS; linha++)
        {
            for (coluna = 0; coluna < NUM_ROWS; coluna++)
            {
                if(entity_grid[linha][coluna].type == plant){
                    std::thread thread_planta (entity_planta);
                    thread_planta.join();
                    //threads.push_back(std::thread(entity_planta));
                }
                if(entity_grid[linha][coluna].type == herbivore){
                    std::thread thread_herbivoro(entity_herbivoro);
                    thread_herbivoro.join();
                    //threads.push_back(std::thread(entity_herbivoro));
                }
                if(entity_grid[linha][coluna].type == carnivore){
                    std::thread thread_carnivoro(entity_carnivoro);
                    thread_carnivoro.join();
                    //threads.push_back(std::thread(entity_carnivoro));
                    
                }
            }
        }
        posicoes.clear();
        /*for (int i=0; i< threads.size(); ++i)
	    {
	    	threads[i].join();
	    }*/

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}