/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CredentialCocoa_h
#define CredentialCocoa_h

#include "CredentialBase.h"
#include <Security/SecBase.h>
#include <wtf/RetainPtr.h>

#if USE(CFNETWORK)
typedef const struct _CFURLCredential* CFURLCredentialRef;
#endif

OBJC_CLASS NSURLCredential;

namespace WebCore {

class Credential : public CredentialBase {
public:
    Credential()
        : CredentialBase()
    {
    }

    Credential(const String& user, const String& password, CredentialPersistence persistence)
        : CredentialBase(user, password, persistence)
    {
    }

    Credential(const Credential&, CredentialPersistence);

#if USE(CFNETWORK)
    explicit Credential(CFURLCredentialRef);
#endif
    explicit Credential(NSURLCredential *);

    bool isEmpty() const;

    bool encodingRequiresPlatformData() const { return m_nsCredential && encodingRequiresPlatformData(m_nsCredential.get()); }

#if USE(CFNETWORK)
    CFURLCredentialRef cfCredential() const;
#endif
    NSURLCredential *nsCredential() const;

    static bool platformCompare(const Credential&, const Credential&);

private:
    static bool encodingRequiresPlatformData(NSURLCredential *);

    mutable RetainPtr<NSURLCredential> m_nsCredential;
};

} // namespace WebCore

#endif // CredentialCocoa_h
