///////////////////////////////////////////////////////////////////////////////
// maxdefense.hh
//
// Compute the set of armos that maximizes defense, within a gold budget,
// with the dynamic method or exhaustive search.
//
///////////////////////////////////////////////////////////////////////////////


#pragma once


#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <vector>


// One armor item available for purchase.
class ArmorItem
{
	//
	public:
		
		//
		ArmorItem
		(
			const std::string& description,
			size_t cost_gold,
			double defense_points
		)
			:
			_description(description),
			_cost_gold(cost_gold),
			_defense_points(defense_points)
		{
			assert(!description.empty());
			assert(cost_gold > 0);
		}
		
		//
		const std::string& description() const { return _description; }
		int cost() const { return _cost_gold; }
		double defense() const { return _defense_points; }
	
	//
	private:
		
		// Human-readable description of the armor, e.g. "new enchanted helmet". Must be non-empty.
		std::string _description;
		
		// Cost, in units of gold; Must be positive
		int _cost_gold;
		
		// Defense points; most be non-negative.
		double _defense_points;
};


// Alias for a vector of shared pointers to ArmorItem objects.
typedef std::vector<std::shared_ptr<ArmorItem>> ArmorVector;


// Load all the valid armor items from the CSV database
// Armor items that are missing fields, or have invalid values, are skipped.
// Returns nullptr on I/O error.
std::unique_ptr<ArmorVector> load_armor_database(const std::string& path)
{
	std::unique_ptr<ArmorVector> failure(nullptr);
	
	std::ifstream f(path);
	if (!f)
	{
		std::cout << "Failed to load armor database; Cannot open file: " << path << std::endl;
		return failure;
	}
	
	std::unique_ptr<ArmorVector> result(new ArmorVector);
	
	size_t line_number = 0;
	for (std::string line; std::getline(f, line); )
	{
		line_number++;
		
		// First line is a header row
		if ( line_number == 1 )
		{
			continue;
		}
		
		std::vector<std::string> fields;
		std::stringstream ss(line);
		
		for (std::string field; std::getline(ss, field, '^'); )
		{
			fields.push_back(field);
		}
		
		if (fields.size() != 3)
		{
			std::cout
				<< "Failed to load armor database: Invalid field count at line " << line_number << "; Want 3 but got " << fields.size() << std::endl
				<< "Line: " << line << std::endl
				;
			return failure;
		}
		
		std::string
			descr_field = fields[0],
			cost_gold_field = fields[1],
			defense_points_field = fields[2]
			;
		
		auto parse_dbl = [](const std::string& field, double& output)
		{
			std::stringstream ss(field);
			if ( ! ss )
			{
				return false;
			}
			
			ss >> output;
			
			return true;
		};
		
		std::string description(descr_field);
		double cost_gold, defense_points;
		if (
			parse_dbl(cost_gold_field, cost_gold)
			&& parse_dbl(defense_points_field, defense_points)
		)
		{
			result->push_back(
				std::shared_ptr<ArmorItem>(
					new ArmorItem(
						description,
						cost_gold,
						defense_points
					)
				)
			);
		}
	}

	f.close();
	
	return result;
}


// Convenience function to compute the total cost and defense in an ArmorVector.
// Provide the ArmorVector as the first argument
// The next two arguments will return the cost and defense back to the caller.
void sum_armor_vector
(
	const ArmorVector& armors,
	int& total_cost,
	double& total_defense
)
{
	total_cost = total_defense = 0;
	for (auto& armor : armors)
	{
		total_cost += armor->cost();
		total_defense += armor->defense();
	}
}


// Convenience function to print out each ArmorItem in an ArmorVector,
// followed by the total kilocalories and protein in it.
void print_armor_vector(const ArmorVector& armors)
{
	std::cout << "*** Armor Vector ***" << std::endl;
	
	if ( armors.size() == 0 )
	{
		std::cout << "[empty armor list]" << std::endl;
	}
	else
	{
		for (auto& armor : armors)
		{
			std::cout
				<< "Ye olde " << armor->description()
				<< " ==> "
				<< "Cost of " << armor->cost() << " gold"
				<< "; Defense points = " << armor->defense()
				<< std::endl
				;
		}
		
		int total_cost;
		double total_defense;
		sum_armor_vector(armors, total_cost, total_defense);
		std::cout
			<< "> Grand total cost: " << total_cost << " gold" << std::endl
			<< "> Grand total defense: " << total_defense
			<< std::endl
			;
	}
}


// Convenience function to print out a 2D cache, composed of an std::vector<std::vector<double>>
// For sanity, will refuse to print a cache that is too large.
// Hint: When running this program, you can redirect stdout to a file,
//	which may be easier to view and inspect than a terminal
void print_2d_cache(const std::vector<std::vector<double>>& cache)
{
	std::cout << "*** 2D Cache ***" << std::endl;
	
	if ( cache.size() == 0 )
	{
		std::cout << "[empty]" << std::endl;
	}
	else if ( cache.size() > 250 || cache[1].size() > 250 )
	{
		std::cout << "[too large]" << std::endl;
	}
	else
	{
		for (std::vector<double> row : cache)
		{
			for ( double value : row )
			{
				std::cout << std::setw(5) << value;
			}
			std::cout << std::endl;
		}
	}
}

// Filter the vector source, i.e. create and return a new ArmorVector
// containing the subset of the armor items in source that match given
// criteria.
// This is intended to:
//	1) filter out armor with zero or negative defense that are irrelevant to our optimization
//	2) limit the size of inputs to the exhaustive search algorithm since it will probably be slow.
//
// Each armor item that is included must have at minimum min_defense and at most max_defense.
//	(i.e., each included armor item's defense must be between min_defense and max_defense (inclusive).
//
// In addition, the the vector includes only the first total_size armor items that match these criteria.
std::unique_ptr<ArmorVector> filter_armor_vector
(
	const ArmorVector& source, 
	double min_defense,
        double max_defense, 
	int total_size
) 

{
    std::unique_ptr<ArmorVector> filtered(new ArmorVector);
    int num_items_found = 0;

    // for loop to go over all armor items until enough valid armor items are found
    for (auto& armor_item : source) 
    {
        double defense = armor_item->defense();
        bool defense_is_in_the_desired_range = (defense >= min_defense) && (defense <= max_defense);

        if (defense_is_in_the_desired_range) {
            filtered->push_back(armor_item);
            num_items_found++;

            // if you've found enough items, break out of the for loop
            if (num_items_found == total_size) {
                break;
            }
        }
    }

    return filtered;
}

// Compute the optimal set of armor items with a dynamic algorithm.
// Specifically, among the armor items that fit within a total_cost gold budget,
// choose the selection of armors whose defense is greatest.
// Repeat until no more armor items can be chosen, either because we've run out of armor items,
// or run out of gold.
std::unique_ptr<ArmorVector> dynamic_max_defense
(
    const ArmorVector& armors, int budget) {
    int num_rows = armors.size() + 1; // add an extra row for no items
    int num_columns = budget + 1;     // add an extra column for budget = 0

    // initialize all values to 0
    std::vector<std::vector<double>> cache(num_rows, std::vector<double> (num_columns, 0));
    for (int i = 0; i < num_rows; i++) {
        for (int j = 0; j < num_columns; j++) {
            cache[i][j] = 0;
        }
    }

    // populate the cache
    for (int i = 1; i < num_rows; i++) {
        int item_index = i - 1; // adjust the index since cache has an extra row
        double item_defense = armors[item_index]->defense();
        int item_cost = armors[item_index]->cost();

        for (int j = 1; j < num_columns; j++) {
            double up = cache[i - 1][j];

            // if the up_left column index is invalid, continue with the next column
            int up_left_column_index = j - item_cost;
            if (up_left_column_index < 0) {
                cache[i][j] = up;
                continue;
            }

            double up_left = cache[i - 1][up_left_column_index];
            double up_left_total = up_left + item_defense;

            double max_defense = std::max(up, up_left_total);
            cache[i][j] = max_defense;
        }
    }

    // rebuild the list of items
    // - if there's a tie, choose the "up" square

    // start at the bottom-right square
    int i = num_rows - 1;
    int j = num_columns - 1;
    ArmorVector items;

    // keep looping until you've reached the top row or left column
    while (i > 0 && j > 0) {
        int item_index = i - 1; // adjust the index since cache has an extra row
        double item_defense = armors[item_index]->defense();
        int item_cost = armors[item_index]->cost();

        double up = cache[i - 1][j];

        // if the up_left column index is invalid, go up
        int up_left_column_index = j - item_cost;
        if (up_left_column_index < 0) {
            i--; // go up
            continue; // continue with the next iteration
        }

        double up_left = cache[i - 1][up_left_column_index];
        double up_left_total = up_left + item_defense;

        if (up < up_left_total) {
            items.push_back(armors[item_index]); // add the current item

            // go up and to the left
            i--;
            j -= item_cost;
        } else { // up >= up_left_total
            i--; // go up
        }
    }

    std::unique_ptr<ArmorVector> final_result = std::make_unique<ArmorVector>(items);
	return final_result;
}

// Compute the optimal set of armor items with an exhaustive search algorithm.
// Specifically, among all subsets of armor items,
// return the subset whose gold cost fits within the total_cost budget,
// and whose total defense is greatest.
// To avoid overflow, the size of the armor items vector must be less than 64.
std::unique_ptr<ArmorVector> exhaustive_max_defense(const ArmorVector& armors, int budget) {
    const int n = armors.size();
    assert(n < 64);
    ArmorVector best;
    double best_defense = 0;
    int max_num = pow(2, n) - 1; // 2^n - 1

    for (int bits = 0; bits <= max_num; bits++) {
        ArmorVector candidate;
        // keep track of the total cost & total defense for the candidate vector,
        // so I don't need to loop through it again later
        int total_cost = 0;
        double total_defense = 0;

        // add particular armor items to the candidate vector
        for (int j = 0; j <= n - 1; j++) {
            // if the rightmost bit after shifting == 1
            if (((bits >> j) & 1) == 1) {
                candidate.push_back(armors[j]);
                total_cost += armors[j]->cost();
                total_defense += armors[j]->defense();
            }
        }

        // update the best set of armor items
        if (total_cost <= budget) {
            if (total_defense > best_defense) {
                best = candidate;
                best_defense = total_defense;
            }
        }
    }

    std::unique_ptr<ArmorVector> final_result = std::make_unique<ArmorVector>(best);
    return final_result;
}
