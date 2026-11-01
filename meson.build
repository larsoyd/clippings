project('clippings', 'cpp', version: '0.1.0', license: 'GPL-3.0-or-later',
    default_options: ['cpp_std=c++20', 'warning_level=2', 'buildtype=debugoptimized'])

qt = import('qt6')
qt_modules = dependency('qt6', modules: ['Core', 'Gui', 'Network', 'Qml', 'Quick'])
layer_shell = dependency('LayerShellQt', method: 'cmake', modules: ['LayerShellQt::Interface'])

capture_sources = ['capture-session.cpp', 'instance-door.cpp', 'screen-grabber.cpp', 'wayfire-client.cpp']
capture_headers = ['capture-session.hpp', 'instance-door.hpp', 'screen-grabber.hpp']
logic_sources = ['annotation.cpp', 'document.cpp', 'export.cpp', 'frozen-desktop.cpp']
logic_headers = ['document.hpp']

program_sources = ['main.cpp', 'application.cpp', 'capture-overlays.cpp', 'document-canvas.cpp', 'live-settings.cpp']
program_headers = ['application.hpp', 'document-canvas.hpp', 'live-settings.hpp']
executable('clippings', [program_sources, capture_sources, logic_sources,
        qt.compile_moc(headers: program_headers + capture_headers + logic_headers, dependencies: qt_modules)],
    dependencies: [qt_modules, layer_shell],
    # a build that isn't installed loads qml from the source tree, so qml edits need no rebuild
    cpp_args: ['-DSOURCE_DIRECTORY="' + meson.current_source_dir() + '"',
               '-DCLIPPINGS_VERSION="' + meson.project_version() + '"'],
    install: true)

install_subdir('qml', install_dir: get_option('datadir') / 'clippings')
install_data('data/clippings.desktop', install_dir: get_option('datadir') / 'applications')

qt_test = dependency('qt6', modules: ['Core', 'Gui', 'Network', 'Test'])
check_core = executable('check-core',
    ['tests/check-core.cpp', logic_sources, qt.compile_moc(headers: ['tests/check-core.hpp'] + logic_headers, dependencies: qt_test)],
    dependencies: [qt_test])
test('core', check_core, env: ['QT_QPA_PLATFORM=offscreen'])

check_capture = executable('check-capture',
    ['tests/check-capture.cpp', capture_sources, 'frozen-desktop.cpp',
     qt.compile_moc(headers: ['tests/check-capture.hpp'] + capture_headers, dependencies: qt_test)],
    dependencies: [qt_test])
test('capture', check_capture, env: ['QT_QPA_PLATFORM=offscreen'], timeout: 60)
