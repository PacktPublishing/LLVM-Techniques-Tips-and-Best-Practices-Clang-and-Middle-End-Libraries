include "Ingredients.td"

/// First, pre-heat oil to 160C
def heat : Action<"heat", deep_fryer>;
def fry_oil : Oil<500>;
def oil_temp : Temperature<160>;
def step_prep_oil : Step<(heat:$action fry_oil:$oil, oil_temp:$temp),
                         no_duration,
                         "$action $oil until $temp">;

/// Mix all the ingredient
def flour : Flour<300>;            // 300g
def milk : Milk<1,1>;              // 1 and 1/4 cup
defm egg : Egg<1>;                 // 1 whole egg
def yeast : Yeast<1,2>;            // 1 and 1/2 tsp yeast
def butter : Butter<3,2>;          // 3 and 1/2 tbsp no salt butter
def sugar : Sugar<2,0>;            // 2 tbsp sugar
def salt  : Salt<0,2>;             // 1/2 tsp salt
def vanilla : VanillaExtract<1,0>; // 1 tsp vanilla extract

def mix : Action<"mix", mixer>;
def step_mixing1 : Step<(mix flour, milk, egg_whole, yeast, butter,
                             sugar, salt, vanilla),
                         Duration<2, minute_unit>> {
  let Note = "stir in low speed";
}

def step_mixing2 : Step<(mix step_mixing1), Duration<10, minute_unit>> {
  let Note = "stir in medium speed";
}

/// Wait the dough to rise
def ferment : Action<"ferment", bowl>;
def step_rising : Step<(ferment step_mixing2), Duration<40, minute_unit>>;

/// Flatten the dough
def flatten : Action<"flatten", rolling_pin>;
def step_flatten : Step<(flatten step_rising), no_duration>;

// Cut into shape
def cut : Action<"cut", cutter>;
def step_cutting : Step<(cut step_flatten), no_duration>;

def fry : Action<"fry", deep_fryer>;
def step_frying : Step<(fry step_prep_oil, step_cutting), Duration<2, minute_unit>>,
                  FinalStep;
