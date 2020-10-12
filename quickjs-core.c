/*
  Copyright (C) 2020 by Shohei YOSHIDA

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h>

#include <quickjs/quickjs.h>

#include <emacs-module.h>

int plugin_is_GPL_compatible;

struct quickjs_context {
    JSRuntime *runtime;
    JSContext *context;
};

static char *retrieve_string(emacs_env *env, emacs_value str, ptrdiff_t *size) {
    *size = 0;

    env->copy_string_contents(env, str, NULL, size);
    char *p = malloc(*size);
    if (p == NULL) {
        *size = 0;
        return NULL;
    }
    env->copy_string_contents(env, str, p, size);

    return p;
}

static emacs_value quickjs_to_elisp(emacs_env *env, JSContext *context, JSValue value) {
    int tag = JS_VALUE_GET_TAG(value);

    switch (tag) {
    case JS_TAG_INT:
        return env->make_integer(env, JS_VALUE_GET_INT(value));
    case JS_TAG_FLOAT64:
        return env->make_float(env, JS_VALUE_GET_FLOAT64(value));
    case JS_TAG_BOOL:
        if (JS_VALUE_GET_BOOL(value)) {
            return env->intern(env, "t");
        }
        return env->intern(env, "nil");
    case JS_TAG_STRING: {
        const char *str = JS_ToCString(context, value);
        emacs_value ret = env->make_string(env, str, strlen(str));
        JS_FreeCString(context, str);
        return ret;
    }
    case JS_TAG_NULL:
    case JS_TAG_UNDEFINED:
        return env->intern(env, "nil");
    case JS_TAG_EXCEPTION:
        // XXX what value return ??
        return env->intern(env, "nil");
    case JS_TAG_OBJECT:
    case JS_TAG_MODULE: {
        if (JS_IsArray(context, value)) {
            JSValue v = JS_GetPropertyStr(context, value, "length");
            uint32_t length = JS_VALUE_GET_INT(v);

            emacs_value Fmake_vector = env->intern(env, "make-vector");
            emacs_value func_args[2] = {
                env->make_integer(env, length),
                env->make_integer(env, 0),
            };
            emacs_value vec = env->funcall(env, Fmake_vector, 2, &func_args[0]);

            for (uint32_t i = 0; i < length; ++i) {
                JSValue elem = JS_GetPropertyUint32(context, value, i);
                env->vec_set(env, vec, i, quickjs_to_elisp(env, context, elem));
            }

            return vec;
        }

        JSPropertyEnum *ptab;
        uint32_t size;
        JS_GetOwnPropertyNames(context, &ptab, &size, value, 3); // XXX flags

        emacs_value Fmake_hash_table = env->intern(env, "make-hash-table");
        emacs_value make_hash_args[] = {
            env->intern(env, ":test"),
            env->intern(env, "equal"),
        };
        emacs_value hash = env->funcall(env, Fmake_hash_table, 2, make_hash_args);
        emacs_value Fputhash = env->intern(env, "puthash");

        for (uint32_t i = 0; i < size; ++i) {
            JSValue v = JS_GetProperty(context, value, ptab[i].atom);
            const char *k = JS_AtomToCString(context, ptab[i].atom);
            emacs_value key = env->make_string(env, k, strlen(k));
            JS_FreeCString(context, k);

            emacs_value ev = quickjs_to_elisp(env, context, v);
            emacs_value puthash_args[] = {key, ev, hash};
            env->funcall(env, Fputhash, 3, puthash_args);
        }

        return hash;
    }
    default:
        break;
    }

    return env->intern(env, "nil");
}

static bool eq_type(emacs_env *env, emacs_value type, const char *type_str) {
    return env->eq(env, type, env->intern(env, type_str));
}

static JSValueConst elisp_to_quickjs(emacs_env *env, JSContext *context, emacs_value value) {
    emacs_value type = env->type_of(env, value);
    if (eq_type(env, type, "integer")) {
        return JS_MKVAL(JS_TAG_INT, env->extract_integer(env, value));
    } else if (eq_type(env, type, "float")) {
        return JS_MKVAL(JS_TAG_FLOAT64, env->extract_float(env, value));
    } else if (eq_type(env, type, "string")) {
        ptrdiff_t size = 0;
        env->copy_string_contents(env, value, NULL, &size);
        char *str = malloc(size);
        env->copy_string_contents(env, value, str, &size);
        JSValueConst ret = JS_NewString(context, str);
        free(str);
        return ret;
    } else if (eq_type(env, type, "vector")) {
        JSValue array = JS_NewArray(context);
        ptrdiff_t size = env->vec_size(env, value);
        for (ptrdiff_t i = 0; i < size; ++i) {
            JSValueConst elem = elisp_to_quickjs(env, context, env->vec_get(env, value, i));
            JS_SetPropertyInt64(context, array, i, elem);
        }

        return array;
    } else if (env->eq(env, value, env->intern(env, "t"))) {
        // XXX false??
        return JS_MKVAL(JS_TAG_BOOL, 1);
    } else {
        // object unsuppoerted yet
        return JS_UNDEFINED;
    }
}

static void quickjs_context_free(void *arg) {
    struct quickjs_context *js_context = (struct quickjs_context *)arg;
    JS_FreeContext(js_context->context);
    JS_FreeRuntime(js_context->runtime);
}

static emacs_value Fquickjs_make_context(emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data) {
    JSRuntime *runtime = JS_NewRuntime();
    JSContext *context = JS_NewContext(runtime);

    struct quickjs_context *js_context = malloc(sizeof(struct quickjs_context));
    if (context == NULL) {
        return env->intern(env, "nil");
    }

    js_context->runtime = runtime;
    js_context->context = context;

    return env->make_user_ptr(env, quickjs_context_free, js_context);
}

static emacs_value Fquickjs_eval_with_context(emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data) {
    struct quickjs_context *js_context = env->get_user_ptr(env, args[0]);

    ptrdiff_t size;
    char *code = retrieve_string(env, args[1], &size);
    if (code == NULL) {
        return env->intern(env, "nil");
    }

    JSValue value = JS_Eval(js_context->context, code, size - 1, "<input>", JS_EVAL_FLAG_STRICT);
    free(code);

    emacs_value ret = quickjs_to_elisp(env, js_context->context, value);
    JS_FreeValue(js_context->context, value);
    return ret;
}

static emacs_value Fquickjs_call(emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data) {
    struct quickjs_context *js_context = env->get_user_ptr(env, args[0]);

    ptrdiff_t size;
    char *func_name = retrieve_string(env, args[1], &size);
    if (func_name == NULL) {
        return env->intern(env, "nil");
    }

    JSValue global = JS_GetGlobalObject(js_context->context);
    JSValue func = JS_GetPropertyStr(js_context->context, global, func_name);
    free(func_name);

    JSValueConst *js_args = NULL;
    ptrdiff_t args_size = env->vec_size(env, args[2]);
    if (args_size > 0) {
        js_args = malloc(args_size * sizeof(JSValueConst));
        for (ptrdiff_t i = 0; i < args_size; ++i) {
            js_args[i] = elisp_to_quickjs(env, js_context->context, env->vec_get(env, args[2], i));
        }
    }

    JSValue value = JS_Call(js_context->context, func, JS_NULL, args_size, js_args);

    if (js_args != NULL) {
        for (ptrdiff_t i = 0; i < nargs - 2; ++i) {
            JS_FreeValue(js_context->context, js_args[i]);
        }
        free(js_args);
    }

    emacs_value ret = quickjs_to_elisp(env, js_context->context, value);
    JS_FreeValue(js_context->context, value);
    JS_FreeValue(js_context->context, func);
    JS_FreeValue(js_context->context, global);
    return ret;
}

static emacs_value Fquickjs_eval(emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data) {
    ptrdiff_t size;
    char *code = retrieve_string(env, args[0], &size);
    if (code == NULL) {
        return env->intern(env, "nil");
    }

    JSRuntime *runtime = JS_NewRuntime();
    JSContext *context = JS_NewContext(runtime);

    JSValue value = JS_Eval(context, code, size - 1, "<input>", JS_EVAL_FLAG_STRICT);
    free(code);

    emacs_value ret = quickjs_to_elisp(env, context, value);

    JS_FreeValue(context, value);

    JS_FreeContext(context);
    JS_FreeRuntime(runtime);

    return ret;
}

static void bind_function(emacs_env *env, const char *name, emacs_value Sfun) {
    emacs_value Qfset = env->intern(env, "fset");
    emacs_value Qsym = env->intern(env, name);
    emacs_value args[] = {Qsym, Sfun};

    env->funcall(env, Qfset, 2, args);
}

static void provide(emacs_env *env, const char *feature) {
    emacs_value Qfeat = env->intern(env, feature);
    emacs_value Qprovide = env->intern(env, "provide");
    emacs_value args[] = {Qfeat};

    env->funcall(env, Qprovide, 1, args);
}

int emacs_module_init(struct emacs_runtime *ert) {
    emacs_env *env = ert->get_environment(ert);

#define DEFUN(lsym, csym, amin, amax, doc, data) bind_function(env, lsym, env->make_function(env, amin, amax, csym, doc, data))

    DEFUN("quickjs-core-make-context", Fquickjs_make_context, 0, 0, "make quickJS context", NULL);
    DEFUN("quickjs-core-eval-with-context", Fquickjs_eval_with_context, 2, 2, "eval string with context", NULL);
    DEFUN("quickjs-core-call", Fquickjs_call, 3, 3, "call function with context", NULL);

    DEFUN("quickjs-core-eval", Fquickjs_eval, 1, 1, "eval string as JavaScript", NULL);

#undef DEFUN

    provide(env, "quickjs-core");
    return 0;
}
