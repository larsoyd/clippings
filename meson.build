project('clippings', 'cpp', version: '0.1.0', license: 'GPL-3.0-or-later',
    default_options: ['cpp_std=c++20', 'warning_level=2', 'buildtype=debugoptimized'])

qt = import('qt6')
qt_modules = dependency('qt6', modules: ['Core', 'Gui', 'Network', 'Qml', 'Quick'])
layer_shell = dependency('LayerShellQt', method: 'cmake', modules: ['LayerShellQt::Interface'])

logic_sources = ['annotation.cpp', 'document.cpp', 'export.cpp', 'frozen-desktop.cpp']
logic_headers = ['document.hpp']

qt_test = dependency('qt6', modules: ['Core', 'Gui', 'Network', 'Test'])
check_core = executable('check-core',
    ['tests/check-core.cpp', logic_sources, qt.compile_moc(headers: ['tests/check-core.hpp'] + logic_headers, dependencies: qt_test)],
    dependencies: [qt_test])
test('core', check_core, env: ['QT_QPA_PLATFORM=offscreen'])
