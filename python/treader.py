#!/usr/bin/python3

import re
import sys
import time
import itertools

from operator import attrgetter


RE_HEADER = re.compile(r'(\d+),(\d+)')
RE_IP = re.compile(r'(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})|\*+')


class Route(object):
    """
    A traceroute record consists of a number of probes.
    """
    def __init__(self):
        self.time_start = None
        self.time_end = None
        self.time_interval = None
        self.counts = None
        self.probes = [] # Series of IP


    def __eq__(self, other):
        if type(other) == type(self):
            return (self.counts == other.counts) and (sorted(self.probes) == sorted(other.probes))


    def __str__(self):
        if self.counts:
            text = 'Time:{0:s}  Hops:{1:s}  IP:{2:s}\n'.format(
                time.strftime('%Y/%m/%d %H:%M:%S', self.time_start), self.counts, self.probes)
        else:
            text = '*\n'
        return text



class Traceroute(object):
    """
    Abstraction of traceroute results.  Consists of a sequence of
    traceroute result, each of which has IP, hop number and timing information.
    """
    def __init__(self):
        self.current_route = None
        self.tr_list = []


    def __str__(self):
        text = 'Change events:\n'
        for r in self.tr_list:
            text += '-------------------------------------------\n'
            text += str(r)
        text += '-------------------------------------------\n'
        text += '\nTotal route change event number:' + str(len(self.tr_list)) + '\n'
        return text


    def data_read(self, data):
        """
        Read the traceroute output file and find the route changes events
        """
        first_line = data.readline()
        if first_line == '':
            print 'ERROR: Traceroute file is empty'
            return None
        pre_route = self._parse_route(first_line)
        self.tr_list.append(pre_route)

        lines = data.readlines()
        for line in lines:
            update_flag = False
            cur_route = self._parse_route(line)
            if pre_route == cur_route:
                continue
            for i in range(len(cur_route.probes)):
                if cur_route.probes[i] != '':
                    if pre_route.probes[i] == '':
                        update_flag = True
                        continue
                    elif pre_route.probes[i] != cur_route.probes[i]:
                        update_flag = False
                        self.tr_list[-1].time_end = cur_route.time_start
                        self.tr_list[-1].time_interval = time.mktime(self.tr_list[-1].time_end) \
                                                         - time.mktime(self.tr_list[-1].time_start)
                        self.tr_list.append(cur_route)
                        pre_route = cur_route
                        break
            if update_flag:
                pre_route.probes = cur_route.probes  # update incomplete route ip

        cur_route = self._parse_route(lines[-1])
        self.tr_list[-1].time_end = cur_route.time_start
        self.tr_list[-1].time_interval = time.mktime(self.tr_list[-1].time_end) \
                                         - time.mktime(self.tr_list[-1].time_start)


    def pop_route_print(self):
        """
        Find the most popular route based on total length of duration
        """
        for r1, r2 in itertools.combinations(self.tr_list, 2):
            self._route_reduce(r1, r2)
        text = '===========================================\n'
        text += 'Most popular route is:\n'
        pop_route = max(self.tr_list, key=attrgetter('time_interval'))
        text += 'Route duration:{0:s}  Hops:{1:s}  IP:{2:s}\n'.format(
                self._convert_seconds(pop_route.time_interval), pop_route.counts, pop_route.probes)
        text += '===========================================\n'
        print text


    def _route_reduce(self, r1, r2):
        """
        Reduce the route change events
        """
        if r1 == r2 and r1.time_start != r2.time_start:
            r1.time_interval += r2.time_interval


    def _parse_route(self, line):
        """
        Parses one line traceroute info into a route
        """
        route = Route()
        # Get headers
        match_dest = RE_HEADER.search(line)
        route.time_start = time.strptime(match_dest.group(1)[0:14],'%Y%m%d%H%M%S')
        route.counts = match_dest.group(2)
        route.probes = RE_IP.findall(line)
        return route

    def _convert_seconds(self, seconds):
        """
        Converts a number of seconds in human readable time
        """
        m, s = divmod(seconds, 60)
        h, m = divmod(m, 60)
        text = '%d hours %02d mins %02d seconds' % (h, m, s)
        return text


def main():
    if len(sys.argv) != 2:
        print 'ERROR: Wrong number of arguments'
        print 'usage: python treade.py <input_file>'
        sys.exit(1)
    # Create parser instance:
    tr = Traceroute()

    with open(str(sys.argv[1]), 'r') as f:
        # Give it some data:
        tr.data_read(f)
        print tr
        tr.pop_route_print()



if __name__ == '__main__':
    main()
