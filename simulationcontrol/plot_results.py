import matplotlib.pyplot as plt
from parse_results import parse_results_returner

data = {
  'parsec-blackscholes': [(1, 2), (2, 3), (3, 4), (4, 5)],
  'parsec-bodytrack': [(1, 2), (2, 3), (3, 4), (4, 5)],
}

colors = {
  'parsec-blackscholes': ('r', (0, ())),
  'parsec-bodytrack': ('b', (0, (1, 1))),
  'parsec-canneal': ('g', (0, (3, 1, 1, 1))),
  'parsec-dedup': ('c', (5, (10, 3))),
  'parsec-fluidanimate': ('m', (0, (3, 5, 1, 5))),
  'parsec-streamcluster': ('y', (0, (5, 5))),
  'parsec-swaptions': ('tab:orange', (0, (5, 1))),
  'parsec-x264': ('tab:brown', (0, (3, 10, 1, 10))),
}

def plot_results(data, x_label, y_label, plot_name=''):
    # Create the plot
    fig, ax = plt.subplots()
    for bench, values in data.items():
      x = [v[0] for v in values]
      y = [v[1] for v in values]
      ax.plot(x, y, label=bench, color=colors[bench][0], linestyle=colors[bench][1], marker='o')

    ax.set(xlabel=x_label, ylabel=y_label, title=plot_name)
    # ax.legend(loc='center left', bbox_to_anchor=(1, 0.5))
            
    plt.savefig('plots/' + x_label + '_' + plot_name + '_' + y_label.replace('°', '') + '.png')
    # close the plt
    plt.close()

    # # Create a separate legend figure and save it as legend.png
    # legend_fig, legend_ax = plt.subplots()
    # for bench, color in sorted(colors.items()):
    #   legend_ax.plot([], [], label=bench, color=color[0], linestyle=color[1], marker='o')
    # legend_ax.legend(markerscale=0.7)
    # legend_ax.axis('off')
    # legend_fig.set_size_inches(fig.get_size_inches()[0]/3, fig.get_size_inches()[1]/2.2)
    # legend_fig.savefig('plots/legend.png', bbox_inches='tight', pad_inches=0)



def make_cores_plot(header, filterstring):
    headers, data = parse_results_returner()
    header_index = headers.index(header)
    name_index = headers.index('name')
    filter_index = headers.index("filter")
    cores_index = headers.index("cores")

    filtered_data = [d for d in data if d[filter_index] == filterstring]
    data = {}
    for d in filtered_data:
      if d[name_index] not in data:
        data[d[name_index]] = [(d[cores_index], d[header_index])]
      else:
        data[d[name_index]].append((d[cores_index], d[header_index]))

    plot_results(data, 'cores', header, filterstring)

def make_freq_plot(header, filterstring, cores):
    headers, data = parse_results_returner()
    header_index = headers.index(header)
    name_index = headers.index('name')
    filter_index = headers.index("filter")
    freq_index = headers.index("clockspeed")
    cores_index = headers.index("cores")

    filtered_data = [d for d in data if "+".join(d[filter_index].split('+')[1:]) == filterstring and d[cores_index] == int(cores)]

    data = {}
    for d in filtered_data:
      if d[name_index] not in data:
        data[d[name_index]] = [(d[freq_index], d[header_index])]
      else:
        data[d[name_index]].append((d[freq_index], d[header_index]))

    plot_results(data, 'freq (GHz)', header, filterstring + '+' + cores)




make_cores_plot('avg resp time (ns)', '1.0GHz+maxFreq+slowDVFS')
make_cores_plot('avg resp time (ns)', '2.0GHz+maxFreq+slowDVFS')
make_cores_plot('avg resp time (ns)', '3.0GHz+maxFreq+slowDVFS')
make_cores_plot('avg resp time (ns)', '4.0GHz+maxFreq+slowDVFS')
make_cores_plot('avg power (W)', '1.0GHz+maxFreq+slowDVFS')
make_cores_plot('avg power (W)', '2.0GHz+maxFreq+slowDVFS')
make_cores_plot('avg power (W)', '3.0GHz+maxFreq+slowDVFS')
make_cores_plot('avg power (W)', '4.0GHz+maxFreq+slowDVFS')
make_cores_plot('energy (J)', '1.0GHz+maxFreq+slowDVFS')
make_cores_plot('energy (J)', '2.0GHz+maxFreq+slowDVFS')
make_cores_plot('energy (J)', '3.0GHz+maxFreq+slowDVFS')
make_cores_plot('energy (J)', '4.0GHz+maxFreq+slowDVFS')
make_cores_plot('peak temperature (°C)', '1.0GHz+maxFreq+slowDVFS')
make_cores_plot('peak temperature (°C)', '2.0GHz+maxFreq+slowDVFS')
make_cores_plot('peak temperature (°C)', '3.0GHz+maxFreq+slowDVFS')
make_cores_plot('peak temperature (°C)', '4.0GHz+maxFreq+slowDVFS')

make_freq_plot('avg resp time (ns)', 'maxFreq+slowDVFS', '1')
make_freq_plot('avg resp time (ns)', 'maxFreq+slowDVFS', '2')
make_freq_plot('avg resp time (ns)', 'maxFreq+slowDVFS', '3')
make_freq_plot('avg resp time (ns)', 'maxFreq+slowDVFS', '4')
make_freq_plot('avg power (W)', 'maxFreq+slowDVFS', '1')
make_freq_plot('avg power (W)', 'maxFreq+slowDVFS', '2')
make_freq_plot('avg power (W)', 'maxFreq+slowDVFS', '3')
make_freq_plot('avg power (W)', 'maxFreq+slowDVFS', '4')
make_freq_plot('energy (J)', 'maxFreq+slowDVFS', '1')
make_freq_plot('energy (J)', 'maxFreq+slowDVFS', '2')
make_freq_plot('energy (J)', 'maxFreq+slowDVFS', '3')
make_freq_plot('energy (J)', 'maxFreq+slowDVFS', '4')
make_freq_plot('peak temperature (°C)', 'maxFreq+slowDVFS', '1')
make_freq_plot('peak temperature (°C)', 'maxFreq+slowDVFS', '2')
make_freq_plot('peak temperature (°C)', 'maxFreq+slowDVFS', '3')
make_freq_plot('peak temperature (°C)', 'maxFreq+slowDVFS', '4')
