import os
from tabulate import tabulate
from resultlib import *
from run import get_threads


def name_from_config(config: str):
    parts = config.split("-")

    return "{}-{}".format(parts[0], parts[1])


def filter_from_run(run: str):
    parts = run.split("_")

    return parts[1]


def clockspeed_from_filter(filter: str):
    def first_float_in_str_to_float(input):
        for i in range(len(input)):
            sub_string = input[: len(input) - i]

            try:
                result = float(sub_string)
                return result
            except ValueError:
                pass

        return None

    return first_float_in_str_to_float(filter)


def parse_results_impl():
    headers = [
        "name",
        "filter",
        "config",
        "tasks",
        "cores",
        "clockspeed",
        "sim. time (ns)",
        "avg resp time (ns)",
        "peak power / thread (W)",
        "avg power (W)",
        "energy (J)",
        "peak temperature (C)",
    ]
    threads = get_threads()

    rows = []
    runs = sorted(list(get_runs()))

    for run in runs:
        if not has_properly_finished(run):
            continue

        if run[-2] != "-":
            raise NotImplementedError(
                "use a more advanced way to get the last (integer) value"
            )

        config = get_config(run)
        energy = get_energy(run)
        if energy == "-":
            energy = 0.0
        else:
            energy = float(energy)
        tasks = get_tasks(run)
        if len(tasks) > 40:
            tasks = tasks[:37] + "..."

        name = name_from_config(config)
        filter = filter_from_run(run)

        clockspeed = clockspeed_from_filter(filter)

        num_cores_index = run[-1]
        num_cores = threads[name][int(num_cores_index) - 1]
        average_peak_temperature = get_average_peak_temperature(run)

        rows.append(
            [
                name,
                filter,
                config,
                tasks,
                int(num_cores),
                clockspeed,
                int(get_total_simulation_time(run)),
                get_average_response_time(run),
                float(get_peak_power_consumption_single_thread(run)),
                float(get_average_power_consumption(run)),
                energy,
                average_peak_temperature,
            ]
        )

    return (headers, rows)


# This function was named by Milan.
def parse_results_returner():
    return parse_results_impl()


def parse_results():
    (headers, rows) = parse_results_impl()

    print(tabulate(rows, headers=headers))


def main():
    parse_results()


if __name__ == "__main__":
    main()
