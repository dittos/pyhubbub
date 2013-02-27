from xml.etree import ElementTree as ET
import _hubbub

__all__ = ['parse']

class TreeHandler(object):
    def __init__(self):
        self.document_node = ET.Element('DOCUMENT_ROOT')
     
    def create_comment(self, data):
        return ET.Comment(data)

    def create_doctype(self, name, public_id, system_id, force_quirks):
        el = ET.Element('<!DOCTYPE>')
        el.text = name
        return el
        
    def create_element(self, ns, tag):
        return ET.Element(tag)

    def create_text(self, data):
        return data

    def append_child(self, parent, child):
        if isinstance(child, basestring):
            if not len(parent):
                if not parent.text:
                    parent.text = u''
                parent.text += child
            else:
                last = parent[-1]
                if not last.tail:
                    last.tail = u''
                last.tail += child
        else:
            parent.append(child)
            child.parent = parent
        return child

    def insert_before(self, parent, child, ref_child):
        index = list(parent).index(ref_child)
        if isinstance(child, basestring):
            if index == 0:
                # <parent>TEXT<ref_child>...</parent>
                # <parent><ref_child>...</parent>
                if not parent.text:
                    parent.text = u''
                parent.text += child
            else:
                # <parent><...><ref_child>...</parent>
                prev = parent[index - 1]
                if not prev.tail:
                    prev.tail = u''
                prev.tail += child
        else:
            parent.insert(index, child)
            child.parent = parent
        return child

    def remove_child(self, parent, child):
        parent.remove(child)
        child.parent = None
        return child

    def clone_node(self, node, deep):
        if isinstance(node, basestring):
            return node
        else:
            clone = ET.Element(node.tag, node.attrib)
            clone.parent = node.parent # ???
            if deep:
                clone.text = node.text
                for child in node:
                    child_clone = self.clone_node(child, deep)
                    child_clone.parent = clone
                    clone.append(child_clone)
            return clone
        raise NotImplementedError()

    def reparent_children(self, node, new_parent):
        if new_parent: # has children
            new_parent[-1].tail += node.text
        else:
            if not new_parent.text:
                new_parent.text = u''
            if node.text is not None:
                new_parent.text += node.text
        node.text = u''
        for child in list(node):
            new_parent.append(child)
            child.parent = new_parent
            node.remove(child)

    def get_parent(self, node, element_only):
        parent = getattr(node, 'parent', None)
        if element_only and parent.tag in ('DOCUMENT_ROOT', '<!DOCTYPE>'):
            return None
        return parent

    def has_children(self, node):
        return bool(node)

    def form_associate(self, form, node):
        node.form = form

    def add_attribute(self, node, ns, name, value):
        node.set(name, value)

    def set_quirks_mode(self, mode):
        #print mode
        pass

    def encoding_change(self, encname):
        #print encname
        pass

def parse(data, encoding=None):
    tree = TreeHandler()
    parser = _hubbub.create_parser(encoding, False, tree)
    parser.parse_chunk(data)
    parser.completed()
    return tree.document_node
