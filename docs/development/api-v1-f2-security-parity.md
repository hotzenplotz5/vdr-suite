# API.V1.F2 – OpenAPI-Security / Reference-JS-Parität (21.11.2026)

**Stand:** F2-Source-Contract auf `work/api-v1-finalisierung-f2-f8`, kein Public-v1-Release.
**Basis:** F1 `c8c971b7cfa01032acc4511be0cc370bfb9f9650`.

## Geschlossene F2-Lücke

Die partielle `api/openapi.json` dokumentiert 60 Pfadvorlagen mit 75
HTTP-Operationen. Zuvor fehlte ein maschinenlesbares `securitySchemes`-Modell.
Jetzt besitzen alle 75 dokumentierten Methoden explizite `security`-Alternativen
und `x-vdr-suite-auth-profile`. Die bestehenden
`x-vdr-suite-authentication`-Grant-Texte bleiben erhalten.

| OpenAPI Scheme | Quellnachweis | Bedeutung |
| --- | --- | --- |
| `browserSession` | `BrowserSessionAuthenticator.h` | Default-Cookie `vdr_suite_session` |
| `browserCsrf` | `BrowserSessionAuthenticator.h` | `X-CSRF-Token`, zusammen mit Browser-Session bei geschützten Mutationen |
| `deviceCredential` | `DeviceCredentialAuthenticator.h` | Header `Authorization: VDR-Suite-Device <id>.<secret>` – **kein Bearer** |
| `managedBasic` | `ManagedBasicAuthenticator.h` | Optionales konfiguriertes HTTP Basic; keine Device-/Browser-Schranken umgehen |
| `pairingToken` | `SecurityHttpGate.h` | `X-VDR-Suite-Pairing-Token` nur im Pairing-Bootstrap |

Anonymous Discovery und Pairing-Beginn bleiben getrennt vom geschützten
Pairing-Status-/Credential-Verfahren. Device-only Wiedergabesitzungen
und Browser-only Device↔Human-Account-Verknüpfung behalten ihre
unterschiedlichen Transportprofile. CSRF wird nicht als eigenständiger
Actor-Nachweis dargestellt.

## Reference-JS-Parität

`clients/reference-js/public-v1-client.js` überträgt bereits die
vom Aufrufer gelieferten Header/Credentials und implementiert die
Pairing-Token-Routen. Es war **kein neuer Login-Mechanismus** notwendig.
Der fokussierte Test `clients/reference-js/tests/test_public_v1_f2_auth_transport.js`
prüft Browser-Cookie-Einstellung/CSRF, Device-Header, anonymen
Pairing-Beginn, tokenbasierte Ausstellung und 401 ohne Retry.
Das lesende `tools/report_public_v1_f2_security.js` zeigt Methodenanzahl,
Security-Profile, unbekannte Scheme-Referenzen und Quellanker.

## Abnahmegrenzen

- Die OpenAPI bleibt **partial**: 75 dokumentierte Operationen sind keine
  Aussage über alle tatsächlichen Runtime-/Addon-Routen.
- OpenAPI-Alternativen zeigen Transportmöglichkeiten. Actor-Typ, Grants,
  Backend-Scope, CSRF und Besitz prüft weiterhin ausschließlich
  `SecurityHttpGate`. Optionale `managedBasic`-Konfiguration ist kein
  generell freigeschalteter Standardauthentifizierungsweg.
- Funktionale Web-/TV-/MLD-Client-Parität wird in F3–F7 bewertet.
- Die zentrale Hardware-Abnahmematrix bleibt unverändert. **Kein**
  Daemon-Neubau, Service-Neustart, realer VDR-Schreibtest, Paket- oder
  UI-Performance-Eingriff durch F2. Keine neuen Freigabe- oder
  Sicherheitsregeln im Server.

**F2-Ergebnis:** Die im bestehenden 60/75-OpenAPI-Inventar dokumentierten
Transport-Verträge sind explizit; das vorhandene Reference-JS-Verhalten
ist zielgerichtet geprüft. API-v1-Release und F3–F8 bleiben separat offen.
