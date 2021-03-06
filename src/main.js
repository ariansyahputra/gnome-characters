// -*- Mode: js; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-
//
// Copyright (c) 2013 Giovanni Campagna <scampa.giovanni@gmail.com>
//
// Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of the GNOME Foundation nor the
//     names of its contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

pkg.initGettext();
pkg.initFormat();
pkg.require({ 'Gdk': '3.0',
              'Gio': '2.0',
              'GLib': '2.0',
              'GObject': '2.0',
              'Gtk': '3.0' });

const {GLib, Gio, GObject, Gtk} = imports.gi;

const Util = imports.util;
const Window = imports.window;

var settings = null;
var application_id = pkg.name;

function initEnvironment() {
    window.getApp = function() {
        return Gio.Application.get_default();
    };
}

var MyApplication = GObject.registerClass({
},class MyApplication extends Gtk.Application {
    _init () {
        super._init({ application_id: application_id,
                      flags: Gio.ApplicationFlags.FLAGS_NONE });

        GLib.set_application_name(_('Characters Application'));
    }

    _onQuit () {
        this.quit();
    }

    _onSearch (action, parameter) {
        const window = new Window.MainWindow({ application: this });
        window.setSearchKeywords(parameter.get_strv());
        window.show();
    }

    vfunc_startup () {
        super.vfunc_startup();

        Util.loadStyleSheet('/org/gnome/Characters/application.css');

        Util.initActions(this,
                         [{ name: 'quit',
                            activate: this._onQuit },
                          { name: 'search',
                            activate: this._onSearch,
                            parameter_type: new GLib.VariantType('as') }]);
        this.set_accels_for_action('app.quit', ['<Primary>q']);

        settings = Util.getSettings('org.gnome.Characters',
                                    '/org/gnome/Characters/');

        log(_("Characters Application started"));
    }

    vfunc_activate () {
        (new Window.MainWindow({ application: this })).show();
    }

    vfunc_shutdown() {
        log(_("Characters Application exiting"));

        super.vfunc_shutdown();
    }
});

function main(argv) {
    initEnvironment();

    return (new MyApplication()).run(argv);
}
