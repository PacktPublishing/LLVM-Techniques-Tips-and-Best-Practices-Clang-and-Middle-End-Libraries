include "Kitchen.td"

class IngredientBase<Unit unit> {
  Unit TheUnit = unit;
  FixedPoint Quantity = FixedPoint<0>;
}

class Milk<int integral, int num_quarter=0> : IngredientBase<cup_unit> {
  let Quantity = NplusQuarter<integral, num_quarter>;
}

class Flour<int weight> : IngredientBase<gram_unit> {
  let Quantity = FixedPoint<weight>;
}

class WholeEgg : IngredientBase<countable_unit> {}
class EggYolk : IngredientBase<countable_unit> {}
class EggWhite : IngredientBase<countable_unit> {}

multiclass Egg<int num> {
  def _whole : WholeEgg {
    let Quantity = FixedPoint<num>;
  }
  def _yolk : EggYolk {
    let Quantity = FixedPoint<num>;
  }
  def _white : EggWhite {
    let Quantity = FixedPoint<num>;
  }
}

class Yeast<int integral, int num_quarter> : IngredientBase<tsp_unit> {
  let Quantity = NplusQuarter<integral, num_quarter>;
}

class Butter<int integral, int num_quarter, bit salt=false> : IngredientBase<tbsp_unit> {
  let Quantity = NplusQuarter<integral, num_quarter>;
  bit WithSalt = salt;
}

class Sugar<int integral, int num_quarter> : IngredientBase<tbsp_unit> {
  let Quantity = NplusQuarter<integral, num_quarter>;
}

class Salt<int integral, int num_quarter> : IngredientBase<tsp_unit> {
  let Quantity = NplusQuarter<integral, num_quarter>;
}

class VanillaExtract<int integral, int num_quarter> : IngredientBase<tsp_unit> {
  let Quantity = NplusQuarter<integral, num_quarter>;
}

class Oil<int volumn> : IngredientBase<milli_liter_unit> {
  let Quantity = FixedPoint<volumn>;
}
