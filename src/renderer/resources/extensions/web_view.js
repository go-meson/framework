// Copyright (c) 2014 Stanislas Polu.
// Copyright (c) 2012 The Chromium Authors.
// See the LICENSE file.

var DocumentNatives = requireNative('document_natives');
var WebViewNatives = requireNative('webview_natives');

/******************************************************************************/
/* ID GENERATOR */
/******************************************************************************/
var _id = 0;
var getNextId = function() {
  return _id++;
}

/******************************************************************************/
/* GLOBAL VALUES */
/******************************************************************************/
var ERROR_MSG_CONTENTWINDOW_NOT_AVAILABLE = '<webview>: ' +
  'contentWindow is not available at this time. It will become available ' +
  'when the page has finished loading.';

const WEB_VIEW_ATTRIBUTE_AUTOSIZE = 'autosize';
const WEB_VIEW_ATTRIBUTE_MAXHEIGHT = 'maxheight';
const WEB_VIEW_ATTRIBUTE_MAXWIDTH = 'maxwidth';
const WEB_VIEW_ATTRIBUTE_MINHEIGHT = 'minheight';
const WEB_VIEW_ATTRIBUTE_MINWIDTH = 'minwidth';
const WEB_VIEW_AtTRIBUTE_SRC = 'src';
const WEB_VIEW_ATTRIBUTE_PARTITION = 'partition';
var AUTO_SIZE_ATTRIBUTES = [
  WEB_VIEW_ATTRIBUTE_AUTOSIZE,
  WEB_VIEW_ATTRIBUTE_MAXHEIGHT,
  WEB_VIEW_ATTRIBUTE_MAXWIDTH,
  WEB_VIEW_ATTRIBUTE_MINHEIGHT,
  WEB_VIEW_ATTRIBUTE_MINWIDTH,
];

var WEB_VIEW_ATTRIBUTES = [
    'allowtransparency',
];

var WEB_VIEW_EVENTS = {
  'did-fail-load': ['url', 'is_top_level', 'error_code', 'error-description'],
  'did-frame-finish-load': ['url', 'is_top_level'],
  'did-start-loading': [],
  'did-stop-loading': [],
  'did-get-redirect-request': ['current_url', 'new_url', 'is_top_level'],
  'console': ['level', 'message', 'line', 'source_id'],
  'new-window': ['target_url', 'frame_name', 'window_container_type', 'disposition'],
  'close': [],
  'crashed': ['process_id', 'reason'],
  'destroyed': [],
  'dialog': ['origin_url', 'accept_lang', 'message_type', 'message_text', 'default_prompt_text'],
  'title-set': ['title', 'explicit_set']
};

/* TODO(spolu): FixMe Chrome 39 */
var PLUGIN_METHOD_ATTACH = '-internal-attach';

class WebViewAttribute {
    constructor(name, my) {
        this.name = name;
        this.value = my.webview_node[name] || '';
        this.my = my;
        this.ignoreMutation = false;
        this.defineProperty();
    }
    getValue() {
        return this.my.webview_node.getAttribute(this.name) || this.value;
    }
    setValue(value) {
        return this.my.webview_node.setAttribute(this.name, value || '');
    }
    setValueIgnoreMutation(value) {
        this.ignoreMutation = true;
        this.setValue(value);
        this.ignoreMutation = false;
    }
    defineProperty() {
        return Object.defineProperty(this.my.webview_node, this.name, {
            get: () => {
                return this.getValue();
            },
            set: (value) => {
                return this.setValue(value);
            },
            enumerable: true
        });
    }

    handleMutation() {}
}

class BooleanAttribute extends WebViewAttribute {
    getValue() {
        return this.my.webview_node.hasAttribute(this.name);
    }
    setValue(value) {
        if (!value) {
            return this.my.webview_node.removeAttribute(this.name);
        } else {
            return this.my.webview_node.setAttribute(this.name, '');
        }
    }
}

class AutosizeDimensionAttribute extends WebViewAttribute {
    getValue() {
        return parseInt(this.my.webview_node.getAttribute(this.name)) || 0;
    }
    handleMutation () {
        if (my.guest_instance_id == null) {
            return undefined;
        }
        return my.api_setAutoSize({
            enableAutoSize: this.my.attributes[WEB_VIEW_ATTRIBUTE_AUTOSIZE].getValue(),
            min: {
                width: parseInt(this.my.attributes[WEB_VIEW_ATTRIBUTE_MINWIDTH].getValue() || 0),
                height: parseInt(this.my.attributes[WEB_VIEW_ATTRIBUTE_MINHEIGHT].getValue() || 0)
            },
            max: {
                width: parseInt(this.my.attributes[WEB_VIEW_ATTRIBUTE_MAXWIDTH].getValue() || 0),
                height: parseInt(this.my.attributes[WEB_VIEW_ATTRIBUTE_MAXHEIGHT].getValue() || 0)
            }
        });
    }
}

class AutosizeAttribute extends BooleanAttribute {
    constructor(my) {
        super(WEB_VIEW_ATTRIBUTE_AUTOSIZE, my);
    }
}
AutosizeAttribute.prototype.handleMutation = AutosizeDimensionAttribute.prototype.handleMutation;
const a = document.createElement('a');

const resolveURL = function (url) {
    if (url === '') return '';
    a.href = url;
    return a.href;
};

class SrcAttribute extends WebViewAttribute {
    constructor(my) {
        super(WEB_VIEW_AtTRIBUTE_SRC, my);
        this.setupMutationObserver();
    }
    getValue() {
        if (this.my.webview_node.hasAttribute(this.name)) {
            return resolveURL(this.my.webview_node.getAttribute(this.name));
        } else {
            return this.value;
        }
    }
    setValueIgnoreMutation(value) {
        super.setValueIgnoreMutation(value);
        return this.observer.takeRecords();
    }
    handleMutation(oldValue, newValue) {
        if (!newValue && oldValue) {
            this.setValueIgnoreMutation(oldValue);
            return undefined;
        }
        return this.parse();
    }
    setupMutationObserver() {
        this.observer = new MutationObserver((mutations) => {
            for (let i = 0, len = mutations.length; i < len; i++) {
                const m = mutations[i];
                const oldValue = m.oldValue;
                const newValue = this.getValue();
                if (oldValue !== newValue) {
                    return;
                }
                this.handleMutation(oldValue, newValue);
            }
        });
        const params = {
            attributes: true,
            attributeOldValue: true,
            attributeFilter: [this.name]
        };
        return this.observer.observe(this.my.webview_node, params);
    }

    parse() {
        //TODO: partition
        if (!this.my.attached || /*!this.my.attributes[WEB_VIEW_ATTRIBUTE_PARTITION].validPartitionId ||*/ !this.getValue()) {
            return;
        }
        if (this.my.guest_instance_id == null) {
            if (this.my.before_first_navigation) {
                this.my.before_first_navigation = false;
                console.log("first_navigation!!");
                this.my.create_guest();
            }
            return;
        }
        my.api_loadUrl(this.getValue());
    }
}

/******************************************************************************/
/* WEBVIEW INTERNAL */
/******************************************************************************/


// ## webview
var webview = function(spec, my) {
  my = my || {};
  spec = spec || {};

  my.webview_node = spec.node;
  my.browser_plugin_node = null;
  my.src_observer = null;

  my.src = null;
  my.attached = false;
  my.element_attached = false;

  my.before_first_navigation = true;
  my.view_instance_id = getNextId();

  my.guest_instance_id = null;
  my.internalInstanceId = null;

  /* Navigation information */
  my.entry_index = 0;
  my.entry_count = 0;
  my.process_id = null;
  my.ignore_next_src = false;
  my.zoom_factor = 1.0;
  my.title = '';

  //
  // _public_
    //
    var webview_mutation_handler;        /* webview_mutation_handler(...); */
    var browser_plugin_mutation_handler; /* browser_plugin_mutation_handler(...); */
    var webview_on_element_resize_handler;

  var attached;                        /* attached([attach_value]) */
  var parse_attributes;                /* parse_attributes(); */
  var reset;                           /* reset(); */

  var api_setAutoSize;                 /* api_setAutoSize(params); */
  var api_go;                          /* api_go(index); */
  var api_back;                        /* api_back(); */
  var api_forward;                     /* api_forward(); */
  var api_canGoBack;                   /* api_canGoBack(); */
  var api_canGoForward;                /* api_canGoForward(); */
  var api_loadUrl;                     /* api_loadUrl(url); */
  var api_reload;                      /* api_reload(ignore_cache); */
  var api_stop;                        /* api_stop(); */
  var api_getProcessId;                /* api_getProcessId(); */
  var api_getZoom;                     /* api_getZoom(); */
  var api_setZoom;                     /* api_setZoom(zoom_factor); */
  var api_find;                        /* api_find(request_id, search_text, options); */
  var api_stopFinding;                 /* api_stopFinding(action); */
  var api_insertCSS;                   /* api_insertCSS(css); */
  var api_executeScript;               /* api_executeScript(script); */
  var api_openDevTools;                /* api_openDevTools(); */
  var api_closeDevTools;               /* api_closeDevTools(); */
  var api_isDevToolsOpened;            /* api_isDevToolsOpened(); */
  var api_getTitle;                    /* api_getTitle(); */
  
  //
  // _private_
  //
    var init;                            /* init(); */
    var setupWebViewAttributes;
    var setupFocusPropagation;

    var event_handler;                   /* event_handler(); */
  var is_plugin_in_render_tree;        /* is_plugin_in_render_tree(); */
  var build_attach_params;             /* build_attach_params(); */
  var attach_window;                   /* attach_window(instance_id); */
  var create_guest;                    /* create_guest(); */
  var attr_src_parse;                  /* attr_src_parse(); */

  //
  // #### _that_
  //
  var that = {};

  /****************************************************************************/
  /* PRIVATE HELPERS */
    /****************************************************************************/
  // ### event_handler
  //
  // Handles an event emitted from the WebViewGuest
  // ```
  // @type  {string} event type
  // @event {object} the event dictionary
  // ```
    event_handler = function(type, event) {
         if(type === 'did-commit-provisional-load') {
            my.entry_index = event.entry_index;
            my.entry_count = event.entry_count;
            my.process_id = event.process_id;
            var old_value = my.webview_node.getAttribute('src');
            var new_value = event.url;

            if(event.is_top_level && (old_value != new_value)) {
                /* Touching the src attribute triggers a navigation. To avoid    */
                /* triggering a page reload on every guest-initiated navigation, */
                /* we use the flag `ignore_next_src` here.                       */
                my.ignore_next_src = true;
                my.webview_node.setAttribute('src', new_value);
            }
            //console.log('EVENT did-commit-provisional-load');
            //console.log(JSON.stringify(event));
        } else if(type === 'zoom-changed') {
            my.zoom_factor = event.new_zoom_factor;
            //console.log('EVENT zoom-changed');
            //console.log(JSON.stringify(event));
        }
        else if(WEB_VIEW_EVENTS[type]) {
            var dom_event = new CustomEvent(type, { cancelable: true });
            WEB_VIEW_EVENTS[type].forEach(function(f) {
                dom_event[f] = event[f];
            });

      if(type === 'dialog') {
        dom_event['ok'] = function(response) {
          dom_event.preventDefault();
          WebViewNatives.JavaScriptDialogClosed(my.guest_instance_id,
                                                true, response || '');
        };
        dom_event['cancel'] = function() {
          dom_event.preventDefault();
          WebViewNatives.JavaScriptDialogClosed(my.guest_instance_id,
                                                false, '');
        };
      }
      else if(type === 'title-set') {
        my.title = event.title;
      }

      var cancel = !my.webview_node.dispatchEvent(dom_event);
      console.log('-------------------------------------------------------')
      console.log('WEB_VIEW_EVENT ' + type + ' ' + (cancel ? 'cancelled' : 'default'));
      console.log(JSON.stringify(event));
      console.log('-------------------------------------------------------')

      if(!cancel) {
        /* If the event is not cancelled be execute the default behaviour for */
        /* the dialog event handler.                                          */
        if(type === 'dialog') {
          dom_event.cancel();
        }
      }
    }
  };

  // ### is_plugin_in_render_tree
  //
  // Returns whether <object> is in the render tree
  is_plugin_in_render_tree = function() {
    /* TODO(spolu): FixMe Chrome 39 */
      //return (typeof my.browser_plugin_node[PLUGIN_METHOD_ATTACH] === 'function');
      return true;
  };

    // ### build_attach_params
    //
    // Returns the attach params for plugin attachment
    build_attach_params = function() {
        const hasProp = {}.hasOwnProperty;
        const params = {
            instanceId: my.view_instance_id,
            userAgentOverride: my.userAgentOverride,
            zoomFactor: DocumentNatives.GetZoomFactor()
        };
        const ref1 = my.attributes;
        for (let attributeName in ref1) {
            if (!hasProp.call(ref1, attributeName)) continue;
            const attribute = ref1[attributeName];
            params[attributeName] = attribute.getValue();
        }
        const css = window.getComputedStyle(my.webview_node, null);
        const elementRect = my.webview_node.getBoundingClientRect();
        params.elementWidth = parseInt(elementRect.width) || parseInt(css.getPropertyValue('width'));
        params.elementHeight = parseInt(elementRect.height) || parseInt(css.getPropertyValue('height'));
        return params;
    };

    // ### attach_window
    //
    // Attaches the guest 
    // ```
    // @instance_id {string} the guest instance id
    // ```
    attach_window = function(instance_id) {
        my.guest_instance_id = instance_id;

        var params = build_attach_params();

        if(!is_plugin_in_render_tree()) {
            return false;
        }

        //return my.browser_plugin_node[PLUGIN_METHOD_ATTACH](instance_id, params);
        if (!my.internalInstanceId) {
            return false;
        }
        return WebViewNatives.AttachWindowGuest(my.internalInstanceId, my.guest_instance_id, params);
    };

  // ### create_guest
  //
  // Triggers the creation of the guest
  create_guest = function() {
    var params = {};

      console.log("create_guest0");
    var instance_id = WebViewNatives.CreateGuest(params);

    /* We register the event handler for events coming from the WebViewGuest. */
      WebViewNatives.SetEventHandler(instance_id, event_handler);
      console.log("create_guest1");

    if(!my.attached) {
      WebViewNatives.DestroyGuest(instance_id);
      return;
    }
    attach_window(instance_id, false);
  };

  // ### attr_src_parse
  //
  // Parses the `src` attribute and navigates if necessary
  attr_src_parse = function() {
    my.src = my.webview_node.getAttribute('src');

    if(!my.src) {
      return;
    }
    if(!my.guest_instance_id) {
      if(my.before_first_navigation) {
        my.before_first_navigation = false;
        /* First navigation. Create Guest. */
        create_guest();
      }
      return;
    }

    /* Navigate to my.src */
    api_loadUrl(my.src);
  };

  /****************************************************************************/
  /* WEBVIEW API */
  /****************************************************************************/
  // ### api_setAutoSize
  //
  // Sets the autosize properties of the webview
  // ```
  // @params {object} autosize params (enabled, min_size, max_size)
  // ```
  api_setAutoSize = function(params) {
    if(!my.guest_instance_id) {
      return;
    }
    WebViewNatives.SetAutoSize(my.guest_instance_id, params);
  };

  // ### api_go
  //
  // Navigates in history to the relative index
  // ```
  // @index {integer} the relative index
  // ```
  api_go = function(index) {
    if(!my.guest_instance_id) {
      return;
    }
    WebViewNatives.Go(my.guest_instance_id, index);
  };

  // ### api_back
  //
  // Navigates back in history
  api_back = function() {
    return that.api_go(-1);
  };

  // ### api_forward
  //
  // Navigates forward in history
  api_forward = function() {
    return that.api_go(1);
  };

  // ### api_canGoBack
  //
  // Whether the webview can go back
  api_canGoBack = function() {
    return my.entry_count > 1 && my.entry_index > 0;
  };

  // ### api_canGoForward
  //
  // Whether the webview can go forward
  api_canGoForward = function() {
    return my.entry_index >= 0 &&
      my.entry_index < (my.entry_count - 1);
  };

  // ### api_loadUrl
  //
  // Loads the specified URL (similar as updating `src`)
  // ```
  // @url {string} the url to load
  // ```
  api_loadUrl = function(url) {
    if(!my.guest_instance_id) {
      return;
    }
    WebViewNatives.LoadUrl(my.guest_instance_id, url);
  };

  // ### api_reload
  //
  // Reloads the webview content
  // ```
  // @ignore_cache {boolean} ignore cache
  // ```
  api_reload = function(ignore_cache) {
    if(!my.guest_instance_id) {
      return;
    }
    WebViewNatives.Reload(my.guest_instance_id, ignore_cache ? true : false);
  };

  // ### api_stop
  //
  // Stops loading
  api_stop = function() {
    if(!my.guest_instance_id) {
      return;
    }
    WebViewNatives.Stop(my.guest_instance_id);
  };

  // ### api_getProcessId
  //
  // Returns the Renderer process id for this webview
  api_getProcessId = function() {
    return my.process_id;
  };

  // ### api_getZoom
  //
  // Returns the current zoom factor
  api_getZoom = function() {
    return my.zoom_factor;
  };

  // ### api_setZoom
  //
  // Sets the zoom factor for this webview
  // ```
  // @zoom_factor {number} the new zoom factor
  // ```
  api_setZoom = function(zoom_factor) {
    if(!my.guest_instance_id) {
      return;
    }
    WebViewNatives.SetZoom(my.guest_instance_id, zoom_factor);
  };

  // ### api_find
  //
  // Starts or continue a find request
  // ```
  // @request_id  {number} request id
  // @search_text {string} the search string
  // @options     {object} forward, match_case, find_next, 
  //                       world_start, medial_capital_as_word_start
  // ```
  api_find = function(request_id, search_text, options) {
    if(!my.guest_instance_id) {
      return;
    }
    var opt = {};
    opt.forward = (options || {}).forward || true;
    opt.match_case = (options || {}).match_case || false;
    opt.find_next = (options || {}).find_next || false;
    opt.word_start = (options || {}).word_start || false;
    opt.medial_capital_as_word_start = 
      (options || {}).medial_capital_as_word_start || false;

    WebViewNatives.Find(my.guest_instance_id, request_id, search_text, opt);
  };
  
  // ### api_stopFinding
  //
  // Stops a find request and perform an action
  // ```
  // @action {string} "clear" | "keep" | "activate"
  // ```
  api_stopFinding = function(action) {
    if(!my.guest_instance_id) {
      return;
    }
    action = action || "clear";
    if(action !== "clear" && action !== "keep" && action !== "activate") {
      return;
    }

    WebViewNatives.StopFinding(my.guest_instance_id, action);
  };

  // ### api_insertCSS
  //
  // Insert CSS in the webview
  // ```
  // @css {string} css text
  // ```
  api_insertCSS = function(css) {
    if(!my.guest_instance_id) {
      return;
    }

    WebViewNatives.InsertCSS(my.guest_instance_id, css);
  };

  // ### api_executeScript
  //
  // Executes a script in the webview
  // ```
  // @script {string} script code
  // ```
  api_executeScript = function(script) {
    if(!my.guest_instance_id) {
      return;
    }

    WebViewNatives.ExecuteScript(my.guest_instance_id, script);
  };

  // ### api_openDevTools
  //
  // Opens the DevTools view for this webview
  api_openDevTools = function() {
    if(!my.guest_instance_id) {
      return;
    }
    WebViewNatives.OpenDevTools(my.guest_instance_id);
  };

  // ### api_closeDevTools
  //
  // Closes the DevTools view for this webview
  api_closeDevTools = function() {
    if(!my.guest_instance_id) {
      return;
    }
    WebViewNatives.CloseDevTools(my.guest_instance_id);
  };

  // ### api_isDevToolsOpened
  //
  // Returns wether the DevTools view is opened or not.
  api_isDevToolsOpened = function() {
    if(!my.guest_instance_id) {
      return;
    }
    return WebViewNatives.IsDevToolsOpened(my.guest_instance_id);
  };

  // ### api_getTitle
  //
  // Returns the current webview title
  api_getTitle = function() {
    return my.title;
  };

  /****************************************************************************/
  /* PUBLIC METHODS */
  /****************************************************************************/
    // ### webview_mutation_handler
    //
    // Handles mutations to the webview and updates the browser_plugin properties
    // accordingly. In turn, updating a BrowserPlugin property will update the 
    // corresponding BrowserPlugin attribute, if necessary. 
    // See BrowserPlugin::UpdateDOMAttribute for more details.
    // ```
    // @name      {string} the attribute name
    // @old_value {value} the old value
    // @new_value {value} the new value
    // ```
    webview_mutation_handler = function(name, old_value, new_value) {
        console.log("HANDLER: " + name + " " + old_value + " " + new_value);
        if (!my.attributes[name] || my.attributes[name].ignoreMutation) {
            return;
        }
        my.attributes[name].handleMutation(old_value, new_value);
    };

    webview_on_element_resize_handler = function(newSize) {
        const resizeEvent = new Event('resize', {
            bubbles: true
        });
        newSize.width = my.webview_node.clientWidth;
        newSize.height = my.webview_node.clientHeight;
        resizeEvent.newWidth = newSize.width;
        resizeEvent.newHeight = newSize.height;
        my.webview_node.dispatchEvent(resizeEvent);
        if (my.guest_instance_id) {
            api_setAutoSize({normal: newSize});
        }
    };

    // ### browser_plugin_mutation_handler
    //
    // ```
    // @name      {string} the attribute name
    // @old_value {value} the old value
    // @new_value {value} the new value
    // ```
    browser_plugin_mutation_handler = function(name, old_value, new_value) {
        /* TODO(spolu): FixMe Chrome 39 */
        if (name == 'internalinstanceid' && !old_value && !!new_value) {
            my.browser_plugin_node.removeAttribute('internalinstanceid');
            my.internalInstanceId = parseInt(new_value);

            // Track when the element resizes using the element resize callback.
            WebViewNatives.RegisterElementResizeCallback(my.internalInstanceId, this.webview_on_element_resize_handler.bind(my));
            if (!my.guest_instance_id) {
                return;
            }
            WebViewNatives.AttachWindowGuest(my.internalInstanceId, my.guest_instance_id, build_attach_params());
        }
    };

  // ### attached
  //
  // Getter/Setter for the attached field
  // ```
  // @attached {boolean}
  // ```
  attached = function(attached_value) {
    if(typeof attached_value !== 'boolean') {
      return my.attached;
    }
    my.attached = attached_value;
    return my.attached;
  }

  // ### parse_attributes
  //
  // Parses the attributes if the element is attached
  parse_attributes = function() {
    if(!my.attached) {
      return;
    }
    attr_src_parse();
  };

  // ### reset
  //
  // Resets the state upon detachment of the element
  reset = function() {
    if(my.guest_instance_id) {
      WebViewNatives.DestroyGuest(my.guest_instance_id);
      my.guest_instance_id = null;
      my.before_first_navigation = true;
    }
  };

    setupWebViewAttributes = function() {
        my.attributes = {};
        my.attributes[WEB_VIEW_ATTRIBUTE_AUTOSIZE] = new AutosizeAttribute(my);
        //TODO: partition
        my.attributes[WEB_VIEW_AtTRIBUTE_SRC] = new SrcAttribute(my);
        //TODO: referrer, useragent, node(meson?)integration, plugins, disablewebsecurity
        //TODO: allowpopups,preload,blinkfeatures,disablelinkfeatures,guestinstance
        const autosizeAttributes = [
            WEB_VIEW_ATTRIBUTE_MAXHEIGHT,
            WEB_VIEW_ATTRIBUTE_MAXWIDTH,
            WEB_VIEW_ATTRIBUTE_MINHEIGHT,
            WEB_VIEW_ATTRIBUTE_MINWIDTH
        ];
        autosizeAttributes.forEach((attribute) => {
            my.attributes[attribute] = new AutosizeDimensionAttribute(attribute, my);
        });
    };
    setupFocusPropagation = function() {
        if (!my.webview_node.hasAttribute('tabIndex')) {
            my.webview_node.setAttribute('tabIndex', -1);
        }
        my.webview_node.addEventListener('focus', () => {
            my.browser_plugin_node.focus();
        });
        my.webview_node.addEventListener('blur', () => {
            my.browser_plugin_node.blur();
        });
    };
  
  /****************************************************************************/
  /* INITIALIZATION */
  /****************************************************************************/
  // ### init
  //
  // Initializes the webview object and registers itself as internal hidden
  // value for the node
  init = function() {
      privates(my.webview_node).internal = that;

      // We create BrowserPlugin as a custom element in order to observe changes
      // to attributes synchronously.
      my.browser_plugin_node = new window.BrowserPlugin();
      privates(my.browser_plugin_node).internal = that;
      const r = my.webview_node.createShadowRoot();
      r.innerHTML = '<!DOCTYPE html><style type="text/css">:host { display: flex; }</style>';
      setupWebViewAttributes();
      setupFocusPropagation();
      /* We create the shadow root for this element and append the browser */
      /* plugin node to it.                                                */
      r.appendChild(my.browser_plugin_node);

      /* Finally triggers the guest creation and first navigation. If the */
      /* element is attached.                                             */
      //parse_attributes();
  };
  
    that.webview_mutation_handler = webview_mutation_handler;
    that.browser_plugin_mutation_handler = browser_plugin_mutation_handler;
    that.webview_on_element_resize_handler = webview_on_element_resize_handler;

    that.attached = attached;
    that.parse_attributes = parse_attributes;
    that.reset = reset;

    that.api_go = api_go;
    that.api_back = api_back;
    that.api_forward = api_forward;
    that.api_canGoBack = api_canGoBack;
    that.api_canGoForward = api_canGoForward;
    that.api_loadUrl = api_loadUrl;
    that.api_reload = api_reload;
    that.api_stop = api_stop;
    that.api_getProcessId = api_getProcessId;
    that.api_getZoom = api_getZoom;
    that.api_setZoom = api_setZoom;
    that.api_find = api_find;
    that.api_stopFinding = api_stopFinding;
    that.api_insertCSS = api_insertCSS;
    that.api_executeScript = api_executeScript;
    that.api_openDevTools = api_openDevTools;
    that.api_closeDevTools = api_closeDevTools;
    that.api_isDevToolsOpened = api_isDevToolsOpened;
    that.api_getTitle = api_getTitle;

    init();

    return that;
};


/******************************************************************************/
/* ELEMENT REGISTRATION */
/******************************************************************************/
// ### registerBrowserPluginElement
//
// Registers browser plugin <object> custom element.
function registerBrowserPluginElement() {
  var proto = Object.create(HTMLObjectElement.prototype);

    proto.createdCallback = function() {
    this.setAttribute('type', 'application/browser-plugin');
    this.setAttribute('id', 'browser-plugin-' + getNextId());
    // The <object> node fills in the <webview> container.
        //this.style.width = '100%';
        //this.style.height = '100%';
        this.style.flex = '1 1 auto';
  };

    proto.attributeChangedCallback = function(name, old_value, new_value) {
    var internal = privates(this).internal;
    if(!internal) {
      return;
    }
    internal.browser_plugin_mutation_handler(name, old_value, new_value);
  };

    proto.attachedCallback = function() {
    // Load the plugin immediately.
    var unused = this.nonExistentAttribute;
  };

  window.BrowserPlugin = DocumentNatives.RegisterElement('browserplugin', { extends: 'object',
                                                                            prototype: proto });

  // Delete the callbacks so developers cannot call them and produce unexpected
  // behavior.
  delete proto.createdCallback;
  delete proto.attachedCallback;
  delete proto.detachedCallback;
  delete proto.attributeChangedCallback;
}

// ### registerWebViewElement
//
// Registers <webview> custom element and sets up method forwarding to the 
// hidden internal value associated with it
function registerWebViewElement() {
  var proto = Object.create(HTMLElement.prototype);

  proto.createdCallback = function() {
    webview({ node: this });
  };

  proto.attributeChangedCallback = function(name, old_value, new_value) {
    var internal = privates(this).internal;
    if(!internal) {
      return;
    }
    internal.webview_mutation_handler(name, old_value, new_value);
  };

  proto.detachedCallback = function() {
    var internal = privates(this).internal;
    if(!internal) {
      return;
    }
    internal.attached(false);
    internal.reset();
  };

    proto.attachedCallback = function() {
        console.log("webview: attachedCallback");
        var internal = privates(this).internal;
        if(!internal) {
            return;
        }
        if(!internal.attached()) {
            internal.attached(true);
            internal.parse_attributes();
        }
    };

  var methods = [
    'go',
    'back',
    'forward',
    'canGoBack',
    'canGoForward',
    'loadUrl',
    'reload',
    'stop',
    'getProcessId',
    'getZoom',
    'setZoom',
    'find',
    'stopFinding',
    'insertCSS',
    'executeScript',
    'openDevTools',
    'closeDevTools',
    'isDevToolsOpened',
    'getTitle',
    /*
    'clearData',
    'print',
    'terminate',
    'getUserAgent',
    'isUserAgentOverridden',
    'setUserAgentOverride'
    */
  ];

  // Forward proto.foo* method calls to WebViewInternal.foo*.
  for(var i = 0; methods[i]; ++i) {
    var createHandler = function(m) {
      return function(var_args) {
        var internal = privates(this).internal;
        return $Function.apply(internal['api_' + m], internal, arguments);
      };
    };
    proto[methods[i]] = createHandler(methods[i]);
  }

  window.WebView = DocumentNatives.RegisterElement('webview', { prototype: proto });

  // Delete the callbacks so developers cannot call them and produce unexpected
  // behavior.
  delete proto.createdCallback;
  delete proto.attachedCallback;
  delete proto.detachedCallback;
  delete proto.attributeChangedCallback;
}

window.addEventListener('readystatechange', function listener(event) {
  if(document.readyState == 'loading')
    return;
  registerBrowserPluginElement();
  registerWebViewElement();
  window.removeEventListener(event.type, listener, true);
}, true);

