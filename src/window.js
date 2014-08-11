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

const GLib = imports.gi.GLib;
const GObject = imports.gi.GObject;
const Gtk = imports.gi.Gtk;
const Lang = imports.lang;
const Params = imports.params;
const Characters = imports.characters;
const Gc = imports.gi.Gc;

const Util = imports.util;

const MainWindow = new Lang.Class({
    Name: 'MainWindow',
    Extends: Gtk.ApplicationWindow,
    Properties: { 'search-active': GObject.ParamSpec.boolean('search-active', '', '', GObject.ParamFlags.READABLE | GObject.ParamFlags.WRITABLE, false) },

    _init: function(params) {
        params = Params.fill(params, { title: GLib.get_application_name(),
                                       default_width: 640,
                                       default_height: 480 });
        this.parent(params);

        this._searchActive = false;

        Util.initActions(this,
                         [{ name: 'new',
                             activate: this._new },
                          { name: 'about',
                            activate: this._about },
                          { name: 'search-active',
                            activate: this._toggleSearch,
                            parameter_type: new GLib.VariantType('b'),
                            state: new GLib.Variant('b', false) }]);

        let builder = new Gtk.Builder();
        builder.add_from_resource('/org/gnome/Characters/main.ui');

        this._headerBar = builder.get_object('main-header');
        this.set_titlebar(this._headerBar);

        let searchBtn = builder.get_object('search-active-button');
        this.bind_property('search-active', searchBtn, 'active',
                           GObject.BindingFlags.SYNC_CREATE |
                           GObject.BindingFlags.BIDIRECTIONAL);
        this._searchBar = builder.get_object('main-search-bar');
        this.bind_property('search-active', this._searchBar, 'search-mode-enabled',
                           GObject.BindingFlags.SYNC_CREATE |
                           GObject.BindingFlags.BIDIRECTIONAL);
        let searchEntry = builder.get_object('main-search-entry');
        this._searchBar.connect_entry(searchEntry);

        let grid = builder.get_object('main-grid');
        let hbox = new Gtk.Box({ orientation: Gtk.Orientation.HORIZONTAL });

        this._categoryListBox = new Gtk.ListBox({});
        this._categoryListBox.get_style_context().add_class('categories');
        this._categoryListRows = {};
        this._mainView = new MainView();

        this._addCategory(_("Punctuations"),
                          'punctuation-page',
                          Gc.Category.PUNCTUATION);
        this._addCategory(_("Arrows"),
                          'arrow-page',
                          Gc.Category.ARROW);
        this._addCategory(_("Bullets"),
                          'bullet-page',
                          Gc.Category.BULLET);
        this._addCategory(_("Picture"),
                          'picture-page',
                          Gc.Category.PICTURE);
        this._addCategory(_("Currencies"),
                          'currency-page',
                          Gc.Category.CURRENCY);
        this._addCategory(_("Math"),
                          'math-page',
                          Gc.Category.MATH);
        // this._addCategory(_("Latin"),
        //                   'latin-page',
        //                   Gc.Category.LATIN);
        this._addCategory(_("Emoticons"),
                          'emoticon-page',
                          Gc.Category.EMOTICON);
        this._mainView.addPage('search-page',
                               Gc.Category.NONE);
        this._mainView.addPage('recent-page',
                               Gc.Category.NONE);

        hbox.pack_start(this._categoryListBox, false, false, 2);
        hbox.pack_start(this._mainView, true, true, 0);
        grid.add(hbox);

        this.add(grid);
        grid.show_all();

        // Due to limitations of gobject-introspection wrt GdkEvent and GdkEventKey,
        // this needs to be a signal handler
        this.connect('key-press-event', Lang.bind(this, this._handleKeyPress));
    },

    _handleRowSelected: function(listBox, row) {
        if (row == null)
            return;

        this._mainView.visible_child_name = row.page_name;
        this._headerBar.title = row.label;
    },

    _addCategory: function(label, page_name, category) {
        let row = new Gtk.ListBoxRow({});
        row.add(new Gtk.Label({ label: label,
                                halign: Gtk.Align.START }));
        row.get_style_context().add_class('category');
        row.label = label;
        row.page_name = page_name;
        this._categoryListBox.add(row);
        this._mainView.addPage(page_name, category);
        this._categoryListBox.connect('row-selected',
                                      Lang.bind(this, this._handleRowSelected));
        return row;
    },

    get search_active() {
        return this._searchActive;
    },

    set search_active(v) {
        if (this._searchActive == v)
            return;

        this._searchActive = v;
        // do something with v
        this.notify('search-active');
    },

    _handleKeyPress: function(self, event) {
        return this._searchBar.handle_event(event);
    },

    _new: function() {
        log(_("New something"));
    },

    _about: function() {
        let aboutDialog = new Gtk.AboutDialog(
            { authors: [ 'Daiki Ueno <dueno@src.gnome.org>' ],
              translator_credits: _("translator-credits"),
              program_name: _("GNOME Characters"),
              comments: _("Character Map"),
              copyright: 'Copyright 2014 Daiki Ueno',
              license_type: Gtk.License.GPL_2_0,
              logo_icon_name: 'org.gnome.Characters',
              version: pkg.version,
              website: 'http://www.example.com/gtk-js-app/',
              wrap_license: true,
              modal: true,
              transient_for: this
            });

        aboutDialog.show();
        aboutDialog.connect('response', function() {
            aboutDialog.destroy();
        });
    },
});

const MainView = new Lang.Class({
    Name: 'MainView',
    Extends: Gtk.Stack,

    _init: function(params) {
        params = Params.fill(params, { hexpand: true,
                                       vexpand: true });
        this.parent(params);
    },

    addPage: function(name, category) {
        let characters = []
        let iter = Gc.enumerate_character_by_category (category);
        while (iter.next ()) {
            characters.push(iter.get());
        }

        let charactersWidget =
            new Characters.CharacterListWidget({ hexpand: true,
                                                 vexpand: true },
                                               characters);
        let viewport = new Gtk.Viewport({});
        viewport.add(charactersWidget);

        let scroll = new Gtk.ScrolledWindow({
            hscrollbar_policy: Gtk.PolicyType.NEVER
        });
        scroll.add(viewport);

        this.add_named(scroll, name);
    }
});
