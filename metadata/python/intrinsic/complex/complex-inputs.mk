# Make file for creating make file rules and targets from a list of test case combinations

# Script for creating make file rule and targets
COMBINATE=./combinate.py

# Rules and targets for the make file that generates complex intrinsic metadata test cases
OUTPUTS = complex-json.targets complex-json.rules complex-xml.targets complex-xml.rules


all: $(OUTPUTS)


.PHONY: clean clean-all


complex-json.targets: complex-inputs.json
	${COMBINATE} --testcases --format json $< -o $@


complex-json.rules: complex-inputs.json
	${COMBINATE} --rules --format json $< -o $@


complex-xml.targets: complex-json.targets
	${COMBINATE} --testcases --format xml $< -o $@


complex-xml.rules: complex-json.targets
	${COMBINATE} --rules --format xml $< -o $@


clean clean-all:
	rm -f $(OUTPUTS)

