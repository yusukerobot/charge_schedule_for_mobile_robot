#pragma once

#include <array>
#include <memory>

namespace nsgaii
{
   struct Individual
   {
      static constexpr size_t max_charge_times = 8;
      std::array<int, max_charge_times> time_chromosome;
      std::array<int, max_charge_times> soc_chromosome;
   };

   class ScheduleNsgaii
   {
   public:
      ScheduleNsgaii();

      void generateParents();
      void generateChildren();
      void generateCombinedPopulation();

      void nonDominatedSorting();
      void crowdingSorting();
      void rankingSelection();
      void crossover();
      void mutation();

   private:
      static constexpr size_t population_size = 20;
      std::array<Individual, population_size> parents;
      std::array<Individual, population_size> children;
      std::array<Individual, 2 * population_size> combind_population;
   };
} // namespace nsgaii
