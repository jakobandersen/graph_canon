import os
import sys

needs_sphinx = '4.0.0'

sys.path.append(os.path.abspath('extensions'))
extensions = [#'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'sphinx.ext.mathjax',
    'sphinx.ext.githubpages',
	'cpp_concepts']

cpp_index_common_prefix = ["graph_canon::"] 
templates_path = ['_templates']
project = u'GraphCanon'
copyright = u'2023, Jakob Lykke Andersen'
author = u'Jakob Lykke Andersen'

with open("../../VERSION") as f:
	version = f.read()
	version = version.strip() # remove the newline
release = version

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# If true, `todo` and `todoList` produce output, else they produce nothing.
todo_include_todos = True

html_theme = 'classic'
html_theme_options = {
    "body_max_width": None
}

intersphinx_mapping = {'https://docs.python.org/': None}
