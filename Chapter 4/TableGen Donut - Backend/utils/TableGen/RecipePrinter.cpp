//===- RecipePrinter.cpp - Skeleton TableGen backend          -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This Tablegen backend emits ...
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"
#include <algorithm>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#define DEBUG_TYPE "recipe-printer"

using namespace llvm;

using AllSteps = std::vector<Record*>;

namespace llvm {
template<>
struct GraphTraits<AllSteps*> {
  using NodeRef = const Init*;
  using ChildIteratorType = typename DagInit::const_arg_iterator;

  static inline Record* toStepRecord(NodeRef N) {
    assert(isa<const DefInit>(N));
    auto* R = cast<const DefInit>(N)->getDef();
    return R->getValue("Action")? R : nullptr;
  }

  static NodeRef getEntryNode(AllSteps* DAG) {
    // Find the step that inheirt from FinalStep
    for (auto* R : *DAG) {
      if (R && R->isSubClassOf("FinalStep"))
        return cast<const Init>(R->getDefInit());
    }
    llvm_unreachable("Forget to mark final steps?");
  }

  static ChildIteratorType child_end(NodeRef N) {
    auto* StepRecord = toStepRecord(N);
    if (StepRecord) {
      auto* ChildDAG = cast<DagInit>(StepRecord->getValueInit("Action"));
      return ChildDAG->arg_end();
    } else {
      // FIXME: This is kind of dirty. We can return null here
      // only because `DagInit::const_arg_iterator` is equal to
      // `SmallVectorImpl<T,...>::const_iterator`, which is equal
      // to `const T*`
      // Not a Step record
      return nullptr;
    }
  }
  static ChildIteratorType child_begin(NodeRef N) {
    auto* StepRecord = toStepRecord(N);
    if (StepRecord) {
      auto* ChildDAG = cast<DagInit>(StepRecord->getValueInit("Action"));
      return ChildDAG->arg_begin();
    } else {
      // Not a Step record
      return child_end(N);
    }
  }
};
} // end namespace llvm

namespace {

// Any helper data structures can be defined here. Some backends use
// structs to collect information from the records.

class RecipePrinter {
private:
  RecordKeeper &Records;

  /// Step record -> index in the linearlized sequence
  std::unordered_map<Record*, unsigned> StepIndicies;

  inline
  void printSimple(raw_ostream& OS, StringRef FieldName,
                   Record* SimpleRecord) {
    OS << SimpleRecord->getValueAsString(FieldName);
  }
  inline
  void printUnit(raw_ostream& OS, Record* UnitRecord) {
    printSimple(OS, "Text", UnitRecord);
  }
  inline
  void printEquipment(raw_ostream& OS, Record* EquipRecord) {
    printSimple(OS, "Name", EquipRecord);
  }

  void printFixedPoint(raw_ostream& OS, Record* FPRecord);

  void printNumeric(raw_ostream& OS, Record* NumericRecord,
                    StringRef ValField, StringRef UnitField);

  inline
  void printTemperature(raw_ostream& OS, Record* TempRecord) {
    printNumeric(OS, TempRecord, "Value", "TempUnit");
  }

  inline
  void printDuration(raw_ostream& OS, Record* DurationRecord) {
    printNumeric(OS, DurationRecord, "Value", "TimeUnit");
  }

  void printIngredient(raw_ostream& OS, Record* IngredientRecord,
                       bool WithQuantity = true);
  void printIngredientList(raw_ostream& OS, ArrayRef<Record*> Ingredients);

  void printAction(raw_ostream& OS, Record* ActionRecord);

  void printStepReference(raw_ostream& OS, Record* StepRecord);

  /// Function to dispatch supported data structures to their printer
  /// functions
  inline
  void printRecord(raw_ostream& OS, Record* R) {
    if (R->isSubClassOf("IngredientBase"))
      printIngredient(OS, R, false);
    else if (R->isSubClassOf("Action"))
      printAction(OS, R);
    else if (R->isSubClassOf("Step"))
      printStepReference(OS, R);
    else if (R->isSubClassOf("Duration"))
      printDuration(OS, R);
    else if (R->isSubClassOf("Temperature"))
      printTemperature(OS, R);
    else if (R->isSubClassOf("Equipment"))
      printEquipment(OS, R);
    else
      llvm_unreachable("Unsupported/Unimplemented record type");
  }

  void printCustomStep(raw_ostream& OS, Record* StepRecord);

public:
  RecipePrinter(RecordKeeper &RK) : Records(RK) {}

  void run(raw_ostream &OS);
}; // emitter class

} // anonymous namespace

void RecipePrinter::printFixedPoint(raw_ostream& OS, Record* FPRecord) {
  auto Integral = FPRecord->getValueAsInt("Integral");
  auto Decimal = FPRecord->getValueAsInt("DecimalPoint");
  assert(Decimal >= 0);

  if (Decimal == 0) OS << Integral;
  else {
    auto Divided = static_cast<unsigned>(std::pow(10, Decimal));
    auto Quotient = Integral / Divided;
    auto Rem = Integral % Divided;
    OS << Quotient << "." << (Rem < 0? -Rem : Rem);
  }
}

void RecipePrinter::printNumeric(raw_ostream& OS, Record* NumericRecord,
                                 StringRef ValField, StringRef UnitField) {
  auto Value = NumericRecord->getValueAsInt(ValField);
  auto* Unit = NumericRecord->getValueAsDef(UnitField);
  printUnit(OS << Value << " ", Unit);
}

void RecipePrinter::printIngredient(raw_ostream& OS,
                                    Record* IngredientRecord,
                                    bool WithQuantity) {
  if (IngredientRecord->isSubClassOf("WholeEgg")) {
    OS << "whole egg";
  } else if (IngredientRecord->isSubClassOf("EggYolk")) {
    OS << "egg yolk";
  } else if (IngredientRecord->isSubClassOf("EggWhite")) {
    OS << "egg white";
  } else if (IngredientRecord->isSubClassOf("VanillaExtract")) {
    OS << "vanilla extract";
  } else if (IngredientRecord->isSubClassOf("Butter")) {
    if (IngredientRecord->getValueAsBit("WithSalt"))
      OS << "salted ";
    OS << "butter";
  } else {
    SmallVector<Record*, 1> SuperClasses;
    IngredientRecord->getDirectSuperClasses(SuperClasses);
    assert(SuperClasses.size() > 0);
    // it's unlikely that there will be more than one direct
    // parent class here...
    OS << SuperClasses[0]->getName().lower();
  }

  if (WithQuantity) {
    printFixedPoint(OS << " ", IngredientRecord->getValueAsDef("Quantity"));
    printUnit(OS << " ", IngredientRecord->getValueAsDef("TheUnit"));
  }
}

void RecipePrinter::printIngredientList(raw_ostream& OS,
                                        ArrayRef<Record*> Ingredients) {
  if (Ingredients.empty()) return;
  else if (Ingredients.size() == 1)
    printRecord(OS, Ingredients.front());
  else if (Ingredients.size() == 2) {
    printRecord(OS, Ingredients.front());
    printRecord(OS << " and ", Ingredients.back());
  } else {
    for (auto i = 0U; i < Ingredients.size(); ++i) {
      printRecord(OS, Ingredients[i]);
      if (i < Ingredients.size() - 1) OS << ", ";
      if (i == Ingredients.size() - 2) OS << "and ";
    }
  }
}

void RecipePrinter::printAction(raw_ostream& OS,
                                Record* ActionRecord) {
  printEquipment(OS << "use ", ActionRecord->getValueAsDef("Using"));
  auto Text = ActionRecord->getValueAsString("Text");
  OS << " to " << Text;
}

void RecipePrinter::printStepReference(raw_ostream& OS,
                                       Record* StepRecord) {
  assert(StepIndicies.count(StepRecord));
  OS << "outcome from (step " << StepIndicies[StepRecord] + 1 << ")";
}

void RecipePrinter::printCustomStep(raw_ostream& OS,
                                    Record* StepRecord) {
  auto CustomFormat = StepRecord->getValueAsString("CustomFormat");
  if (!CustomFormat.contains('$'))
    OS << CustomFormat;

  auto* ActionDAG = StepRecord->getValueAsDag("Action");
  // Map from DAG labels to its Record instance
  StringMap<Init*> ClauseMap;

  // Operator's label
  ClauseMap.insert({ActionDAG->getNameStr(), ActionDAG->getOperator()});
  // Operands' labels
  for (auto i = 0U; i < ActionDAG->arg_size(); ++i) {
    auto ArgLabel = ActionDAG->getArgNameStr(i);
    if (ArgLabel.size() > 0) {
      ClauseMap[ArgLabel] = ActionDAG->getArg(i);
    }
  }

  SmallVector<StringRef, 8> Parts;
  CustomFormat.split(Parts, ' ');
  for (auto i = 0U; i < Parts.size(); ++i) {
    auto Part = Parts[i].trim();
    if (Part.startswith("$")) {
      Part = Part.substr(1);
      if (ClauseMap.count(Part)) {
        auto* Clause = ClauseMap[Part];
        if (auto* Def = dyn_cast<DefInit>(Clause)) {
          printRecord(OS, Def->getDef());
        } else {
          OS << *Clause;
        }
      } else
        OS << Part;
    } else {
      OS << Part;
    }
    if (i < Part.size() - 1)
      OS << " ";
  }
}

void RecipePrinter::run(raw_ostream &OS) {
  //emitSourceFileHeader("Delicious Recipes", OS);
  auto Steps = Records.getAllDerivedDefinitions("Step");

  // Linearlize all the steps
  SmallVector<Record*, 8> StepRecords;
  SmallVector<Record*, 8> UsedIngredients;
  for (const Init* StepOrIngredient : post_order(&Steps)) {
    assert(isa<const DefInit>(StepOrIngredient));
    auto* SIRecord = cast<const DefInit>(StepOrIngredient)->getDef();
    // If `StepOrIngredient` is an ingredient
    // then it's an ingredient that _actually_ got used instead. Which
    // helps us filter out ingredient records that never used by any steps
    if (SIRecord->isSubClassOf("Step")) {
      StepRecords.push_back(SIRecord);
      StepIndicies.insert({SIRecord, StepRecords.size() - 1});
    } else if (SIRecord->isSubClassOf("IngredientBase")){
      UsedIngredients.push_back(SIRecord);
    }
  }

  /// Print ingredient section
  unsigned Idx = 1;
  OS << "=======Ingredients=======\n";
  for (auto* Ingredient : UsedIngredients) {
    printIngredient(OS << Idx++ << ". ", Ingredient);
    OS << "\n";
  }

  /// Print steps
  Idx = 1;
  OS << "\n=======Instructions=======\n";
  for (auto* StepRecord : StepRecords) {
    OS << Idx++ << ". ";
    if (StepRecord->getValue("CustomFormat") &&
        !StepRecord->isValueUnset("CustomFormat") &&
        StepRecord->getValueAsString("CustomFormat").size() > 0) {
      printCustomStep(OS, StepRecord);
    } else {
      // First, the verb part...
      auto* ActionDAG = StepRecord->getValueAsDag("Action");
      assert(isa<DefInit>(ActionDAG->getOperator()));
      auto* Action = cast<DefInit>(ActionDAG->getOperator())->getDef();
      assert(Action->isSubClassOf("Action"));
      printAction(OS, Action);

      // Then it's the ingredients...
      SmallVector<Record*, 4> Ingredients;
      for (auto* Arg : ActionDAG->getArgs())
        if (auto* D = dyn_cast<DefInit>(Arg))
          Ingredients.push_back(D->getDef());
      printIngredientList(OS << " ", Ingredients);
      OS << ".";

      // And ended with note, if any
      if (StepRecord->getValue("Note") && !StepRecord->isValueUnset("Note")) {
        OS << " " << StepRecord->getValueAsString("Note") << ".";
      }
    }
    OS << "\n";
  }
}

namespace llvm {

// The only thing that should be in the llvm namespace is the
// emitter entry point function.

void EmitRecipe(RecordKeeper &RK, raw_ostream &OS) {
  // Instantiate the emitter class and invoke run().
  RecipePrinter(RK).run(OS);
}

} // namespace llvm
