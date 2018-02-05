from docutils import nodes
from docutils.parsers.rst import Directive

class Node(nodes.Part, nodes.Inline, nodes.FixedTextElement):
	pass

def visit_node(self, node):
	self.body.append(R'<div style="font-weight: bold; font-size: 110%;">')

def depart_node(self, node):
	self.body.append(R'</div><hr style="margin-top: 0px; border: none; border-top: 1px solid #8c8c8c; width: 30em; display: inline-block;" />')

class CPPHeader(Directive):
	def run(self):
		n = Node(text=self.txt)
		return [n]

class AssocTypes(CPPHeader):
	txt = "Associated Types"

class Notation(CPPHeader):
	txt = "Notation"

class ValidExpr(CPPHeader):
	txt = "Valid Expressions"

def setup(app):
	app.add_node(Node, html=(visit_node, depart_node))
	app.add_directive("assoc_types", AssocTypes)
	app.add_directive("notation", Notation)
	app.add_directive("valid_expr", ValidExpr)
	return {
		'version': '0.1',
		'parallel_read_safe': True,
		'parallel_write_safe': True,
	}
