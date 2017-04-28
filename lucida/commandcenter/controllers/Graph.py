from Service import Service
from Queue import Queue


class Node(object):
    # Constructor.
    def __init__(self, service_name, to_indices=[]):
        if to_indices is None:
            to_indices = []
        self.service_name = service_name
        self.to_indices = to_indices

    def to_string(self):
        rtn = [self.service_name, str(len(self.to_indices))]
        for to_index in self.to_indices:
            rtn.append(str(to_index))
        return str(rtn)


class Graph(object):
    # Constructor.
    def __init__(self, node_list):
        self.node_list = node_list
        # Start index is always initialized to 0
        # TODO: get rid of start_index and pass it into ThriftClient.infer()
        # since it is only used when traversing through the graph
        self.start_index = 0
        # Validate.
        global_has_seen = set()
        start_node = self.get_node(self.start_index)
        fringe = Queue()
        has_seen = set()
        fringe.put(start_node)
        has_seen.add(start_node)
        while not fringe.empty():
            curr_node = fringe.get()
            for to_index in curr_node.to_indices:
                to_node = self.get_node(to_index)
                if to_node in has_seen:
                    if 'DCM' not in curr_node.service_name:
                        print 'Invalid graph: cyclic without decision node'
                        exit()
                else:
                    fringe.put(to_node)
                    has_seen.add(to_node)
        global_has_seen = has_seen.union(global_has_seen)
        if len(global_has_seen) < len(node_list):
            print 'Invalid graph: unconnected'
        # Create a set of service names for fast look-up.
        self.service_names = set()
        for node in node_list:
            self.service_names.add(node.service_name)

    def get_node(self, index):
        if index  < 0 or index >= len(self.node_list):
            print 'Invalid index'
            exit()
        else:
            return self.node_list[index]

    def get_next_index(self, curr_node, next_service_name):
        for index in curr_node.to_indices:
            if self.get_node(index).service_name == next_service_name:
                    return index
        print 'Invalid next service ' + next_service_name
        exit()

    def to_string(self):
        rtn = ''
        for node in self.node_list:
            rtn += node.to_string()
            rtn += ', '
        rtn += 'and start index: '
        rtn += str(self.start_index)
        return rtn

    def has_service(self, service_name):
        return service_name in self.service_names
