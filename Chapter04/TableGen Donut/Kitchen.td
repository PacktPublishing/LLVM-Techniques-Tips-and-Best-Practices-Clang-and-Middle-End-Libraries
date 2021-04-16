/// Number format
class FixedPoint<int integral, int decimal=0> {
  int Integral = integral;
  int DecimalPoint = decimal;
}

class NplusQuarter<int n, bits<2> num_quarter> : FixedPoint<?, 2> {
  int Part1 = !mul(n, 100);
  int Part2 = !mul(25, !cast<int>(num_quarter{1...0}));
  let Integral = !add(Part1, Part2);
}

/// Units
class Unit<bit imperial, string text> {
  bit Imperial = imperial;
  string Text = text;
}

def ounce_unit        : Unit<true, "oz">;
def pound_unit        : Unit<true, "lb">;
def gram_unit         : Unit<false, "g">;
def kilo_gram_uint    : Unit<false, "kg">;

def cup_unit          : Unit<true, "cup">;
def tbsp_unit         : Unit<true, "tbsp">;
def tsp_unit          : Unit<true, "tsp">;
def liter_unit        : Unit<false, "L">;
def milli_liter_unit  : Unit<false, "ml">;

def countable_unit    : Unit<false, "">;

def minute_unit       : Unit<false, "minute">;
def hour_unit         : Unit<false, "hr">;

def celsius_unit      : Unit<false, "C">;
def fahrenheit_unit   : Unit<true, "F">;

/// Time
class Duration<int n, Unit unit = minute_unit> {
  int Value = n;
  Unit TimeUnit = unit;
}

def no_duration : Duration<-1>;

/// Temperature
class Temperature<int v, Unit unit = celsius_unit> {
  int Value = v;
  Unit TempUnit = unit;
}

/// Gears
class Equipment<string name> {
  string Name = name;
}

def no_equipment  : Equipment<"">;
def mixer         : Equipment<"mixer">;
def bowl          : Equipment<"bowl">;
def rolling_pin   : Equipment<"rolling pin">;
def cutter        : Equipment<"cutter">;
def deep_fryer    : Equipment<"deep fryer">;

/// Actions
class Action<string text, Equipment gear = no_equipment> {
  string Text = text;
  Equipment Using = gear;
}

/// Procedure
class Step<dag action, Duration duration = no_duration, string custom_format = ""> {
  dag Action = action;
  Duration TheDuration = duration;
  string CustomFormat = custom_format;
  string Note;
}

// Just as a marker for the last step
class FinalStep {}
