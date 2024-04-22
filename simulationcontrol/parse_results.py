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


def parse_results_impl():
    headers = [
        "name",
        "filter",
        "config",
        "tasks",
        "cores",
        "sim. time (ns)",
        "avg resp time (ns)",
        "resp times (ns)",
        "peak power / thread (W)",
        "avg power (W)",
        "energy (J)",
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
        tasks = get_tasks(run)
        if len(tasks) > 40:
            tasks = tasks[:37] + "..."

        name = name_from_config(config)
        filter = filter_from_run(run)

        num_cores_index = run[-1]
        num_cores = threads[name][int(num_cores_index) - 1]

        rows.append(
            [
                name,
                filter,
                config,
                tasks,
                num_cores,
                "{:,}".format(get_total_simulation_time(run)),
                "{:,}".format(get_average_response_time(run)),
                "  ".join("{:,}".format(r) for r in get_individual_response_times(run)),
                "{:.2f}".format(get_peak_power_consumption_single_thread(run)),
                "{:.2f}".format(get_average_power_consumption(run)),
                "{:.2f}".format(energy) if energy != "-" else "-",
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
