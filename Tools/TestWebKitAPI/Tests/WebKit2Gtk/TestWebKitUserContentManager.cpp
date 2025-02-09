/*
 * Copyright (C) 2013 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "WebKitTestServer.h"
#include "WebViewTest.h"
#include <cstdarg>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <wtf/gobject/GRefPtr.h>

class UserContentManagerTest : public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(UserContentManagerTest);

    UserContentManagerTest()
        : WebViewTest(WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(webkit_user_content_manager_new())))
    {
        // A reference is leaked when passing the result of webkit_user_content_manager_new()
        // directly to webkit_web_view_new_with_user_content_manager() above. Adopting the
        // reference here avoids the leak.
        m_userContentManager = adoptGRef(webkit_web_view_get_user_content_manager(m_webView));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(m_userContentManager.get()));
    }

    GRefPtr<WebKitUserContentManager> m_userContentManager;
};

static WebKitTestServer* kServer;

// These are all here so that they can be changed easily, if necessary.
static const char* kStyleSheetHTML = "<html><div id=\"styledElement\">Sweet stylez!</div></html>";
static const char* kInjectedStyleSheet = "#styledElement { font-weight: bold; }";
static const char* kStyleSheetTestScript = "getComputedStyle(document.getElementById('styledElement'))['font-weight']";
static const char* kStyleSheetTestScriptResult = "bold";

static void testWebViewNewWithUserContentManager(Test* test, gconstpointer)
{
    GRefPtr<WebKitUserContentManager> userContentManager1 = adoptGRef(webkit_user_content_manager_new());
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(userContentManager1.get()));
    GRefPtr<WebKitWebView> webView1 = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(userContentManager1.get()));
    g_assert(webkit_web_view_get_user_content_manager(webView1.get()) == userContentManager1.get());

    GRefPtr<WebKitWebView> webView2 = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_assert(webkit_web_view_get_user_content_manager(webView2.get()) != userContentManager1.get());
}

static bool isStyleSheetInjectedForURLAtPath(WebViewTest* test, const char* path)
{
    test->loadURI(kServer->getURIForPath(path).data());
    test->waitUntilLoadFinished();

    GUniqueOutPtr<GError> error;
    WebKitJavascriptResult* javascriptResult = test->runJavaScriptAndWaitUntilFinished(kStyleSheetTestScript, &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());

    GUniquePtr<char> resultString(WebViewTest::javascriptResultToCString(javascriptResult));
    return !g_strcmp0(resultString.get(), kStyleSheetTestScriptResult);
}

static void fillURLListFromPaths(char** list, const char* path, ...)
{
    va_list argumentList;
    va_start(argumentList, path);

    int i = 0;
    while (path) {
        // FIXME: We must use a wildcard for the host here until http://wkbug.com/112476 is fixed.
        // Until that time patterns with port numbers in them will not properly match URLs with port numbers.
        list[i++] = g_strdup_printf("http://*/%s*", path);
        path = va_arg(argumentList, const char*);
    }
}

static void removeOldInjectedStyleSheetsAndResetLists(WebKitUserContentManager* userContentManager, char** whitelist, char** blacklist)
{
    webkit_user_content_manager_remove_all_style_sheets(userContentManager);

    while (*whitelist) {
        g_free(*whitelist);
        *whitelist = 0;
        whitelist++;
    }

    while (*blacklist) {
        g_free(*blacklist);
        *blacklist = 0;
        blacklist++;
    }
}

static void testUserContentManagerInjectedStyleSheet(UserContentManagerTest* test, gconstpointer)
{
    char* whitelist[3] = { 0, 0, 0 };
    char* blacklist[3] = { 0, 0, 0 };

    removeOldInjectedStyleSheetsAndResetLists(test->m_userContentManager.get(), whitelist, blacklist);

    // Without a whitelist or a blacklist all URLs should have the injected style sheet.
    static const char* randomPath = "somerandompath";
    g_assert(!isStyleSheetInjectedForURLAtPath(test, randomPath));
    WebKitUserStyleSheet* styleSheet = webkit_user_style_sheet_new(kInjectedStyleSheet, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_STYLE_LEVEL_USER, nullptr, nullptr);
    webkit_user_content_manager_add_style_sheet(test->m_userContentManager.get(), styleSheet);
    webkit_user_style_sheet_unref(styleSheet);
    g_assert(isStyleSheetInjectedForURLAtPath(test, randomPath));

    removeOldInjectedStyleSheetsAndResetLists(test->m_userContentManager.get(), whitelist, blacklist);

    fillURLListFromPaths(blacklist, randomPath, 0);
    styleSheet = webkit_user_style_sheet_new(kInjectedStyleSheet, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_STYLE_LEVEL_USER, nullptr, blacklist);
    webkit_user_content_manager_add_style_sheet(test->m_userContentManager.get(), styleSheet);
    webkit_user_style_sheet_unref(styleSheet);
    g_assert(!isStyleSheetInjectedForURLAtPath(test, randomPath));
    g_assert(isStyleSheetInjectedForURLAtPath(test, "someotherrandompath"));

    removeOldInjectedStyleSheetsAndResetLists(test->m_userContentManager.get(), whitelist, blacklist);

    static const char* inTheWhiteList = "inthewhitelist";
    static const char* notInWhitelist = "notinthewhitelist";
    static const char* inTheWhiteListAndBlackList = "inthewhitelistandblacklist";

    fillURLListFromPaths(whitelist, inTheWhiteList, inTheWhiteListAndBlackList, 0);
    fillURLListFromPaths(blacklist, inTheWhiteListAndBlackList, 0);
    styleSheet = webkit_user_style_sheet_new(kInjectedStyleSheet, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES, WEBKIT_USER_STYLE_LEVEL_USER, whitelist, blacklist);
    webkit_user_content_manager_add_style_sheet(test->m_userContentManager.get(), styleSheet);
    webkit_user_style_sheet_unref(styleSheet);
    g_assert(isStyleSheetInjectedForURLAtPath(test, inTheWhiteList));
    g_assert(!isStyleSheetInjectedForURLAtPath(test, inTheWhiteListAndBlackList));
    g_assert(!isStyleSheetInjectedForURLAtPath(test, notInWhitelist));

    // It's important to clean up the environment before other tests.
    removeOldInjectedStyleSheetsAndResetLists(test->m_userContentManager.get(), whitelist, blacklist);
}

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    soup_message_set_status(message, SOUP_STATUS_OK);
    soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, kStyleSheetHTML, strlen(kStyleSheetHTML));
    soup_message_body_complete(message->response_body);
}

void beforeAll()
{
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    Test::add("WebKitWebView", "new-with-user-content-manager", testWebViewNewWithUserContentManager);
    UserContentManagerTest::add("WebKitUserContentManager", "injected-style-sheet", testUserContentManagerInjectedStyleSheet);
}

void afterAll()
{
    delete kServer;
}
