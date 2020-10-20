;;; quickjs.el --- quickjs for Emacs Lisp

;; Copyright (C) 2020 by Shohei YOSHIDA

;; Author: Shohei YOSHIDA <syohex@gmail.com>
;; URL: https://github.com/syohex/emacs-quickjs
;; Version: 0.01
;; Package-Requires: ((emacs "27.1"))

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;;; Code:

(require 'quickjs-core)

;;;###autoload
(defun quickjs-eval (str)
  (quickjs-core-eval str))

;;;###autoload
(defun quickjs-make-context ()
  (quickjs-core-make-context))

(defun quickjs-eval-with-context (ctx str)
  (quickjs-core-eval-with-context ctx str))

(defun quickjs-call (ctx func &rest args)
  (quickjs-core-call ctx (symbol-name func) (vconcat args)))

(provide 'quickjs)

;;; quickjs.el ends here
