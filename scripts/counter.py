#!/usr/bin/env python3
#
# Script to tabulate counts such as the number of cases that passed or failed
#
# TODO: Remove the Boolean switches that are superseded by action choices

import os
import pickle


def increment_handler(counter, name):
    """Increment the specified counter by one."""
    counter[name] += 1
    return counter[name]


def decrement_handler(counter, name):
    """Decrement the specified counter by one."""
    counter[name] -= 1
    return counter[name]


def value_handler(counter, name):
    """Return the counter value to the caller of this script."""
    print("%d" % counter[name])


def reset_handler(counter, name):
    """Reset the specified counter to zero."""
    counter[name] = 0
    #print(f'Reset counter: {counter}, name: {name}')
    return counter[name]


def print_handler(counter, name):
    """Print the specified counter."""
    print("{counter}: {value}".format(counter=name, value=counter[name]))


# Table of actions and handlers for performing the action
action_table = {
    'increment': {'handler': increment_handler, 'change_flag': True},
    'decrement': {'handler': decrement_handler, 'change_flag': True},
    'value': {'handler': value_handler, 'change_flag': False},
    'reset': {'handler': reset_handler, 'change_flag': True},
    'print': {'handler': print_handler, 'change_flag': False},
}


if __name__ == '__main__':

    from argparse import ArgumentParser
    parser = ArgumentParser(description='Tabulate counts that persist across invocations of this script')
    parser.add_argument('-s', '--store', default='counter.pkl', help='file that stores the counters')
    parser.add_argument('-c', '--count', default='passed', help='variable name of the counter')
    parser.add_argument('-p', '--print', action='store_true', help='print the counter')
    parser.add_argument('-r', '--reset', action='store_true', help='reset the counter')
    parser.add_argument('-f', '--format', help='format string for printing the counters')
    parser.add_argument('-a', '--action', choices=['increment', 'decrement', 'reset', 'print', 'value'], help='operation to perform on the counter')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()
    if args.debug: print(args)

    if os.path.isfile(args.store):
        with open(args.store, 'rb') as file:
            counter = pickle.load(file)
    else:
        counter = {}

    change_flag = False

    if args.action:
        action = action_table.get(args.action, None)
        if action:
            if args.debug: print(action)
            result = action['handler'](counter, args.count)
            change_flag = action['change_flag']

    if args.reset:
        reset_handler(counter, args.count)
        change_flag = True

    if args.format:
        #print(counter)
        print(args.format.format(**counter))
        #change_flag = False

    elif args.print:
        print_handler(counter, args.count)
        #change_flag = False

    if change_flag:
        with open(args.store, 'wb') as file:
            if args.debug: print(f'Writing counter: {counter}')
            pickle.dump(counter, file)

