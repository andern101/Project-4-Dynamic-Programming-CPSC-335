

#include "maxdefense.hh"
#include "timer.hh"


int main()
{
	auto all_armors = load_armor_database("armor.csv");
	assert( all_armors );
  std::vector <int> Item_Num = {1,2,3,4,5,10,15,20,25};

	std::cout << "\nStarting to collect Dynamic Algorithm measurements\n";
	for(auto i = 0; i < Item_Num.size();i++)
	{
	auto filtered_armors = filter_armor_vector(*all_armors, 1, 2500, Item_Num[i]);
	Timer time;
	std::cout << Item_Num[i] << " Items: " << time.elapsed() << "s\n";
	dynamic_max_defense(*filtered_armors, 5000);
	time.reset();
	}

	std::cout << "\nStarting to collect Exhuaustive Optimization Algorithm measurements\n";
	for(auto i = 0; i < Item_Num.size();i++)
	{
	auto filtered_armors = filter_armor_vector(*all_armors, 1, 2500, Item_Num[i]);
	Timer time;
	std::cout << Item_Num[i] << " Items: " << time.elapsed() << "s\n";
	exhaustive_max_defense(*filtered_armors, 5000);
	time.reset();
	}

	std::cout << "Finished";

	return 0;
}
