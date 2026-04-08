# Production Deployment Guide - Security, Upgrades, and Rollback

> **SSOT**: This document is part of the [Production Deployment Guide](PRODUCTION_GUIDE.md).

## 6. Security Hardening

### Security Checklist

**Application Security**:
- [ ] All inputs validated (SQL injection, XSS, command injection)
- [ ] Parameterized SQL queries (no string concatenation)
- [ ] Output encoding for HTML/JavaScript/SQL
- [ ] CSRF protection enabled
- [ ] Rate limiting on all endpoints
- [ ] Authentication required for sensitive endpoints
- [ ] Authorization checks on all resources
- [ ] Secrets never hardcoded (use env vars or secret managers)
- [ ] Error messages don't leak sensitive information
- [ ] Logging doesn't log passwords, tokens, or PII

**Container Security**:
- [ ] Non-root user (USER 1000)
- [ ] Read-only root filesystem (where possible)
- [ ] Minimal base image (Alpine/distroless)
- [ ] No secrets in image layers
- [ ] Image scanned for vulnerabilities (Trivy, Snyk)
- [ ] Drop all capabilities, add only required ones
- [ ] Resource limits set (CPU, memory)
- [ ] Security context configured

**Network Security**:
- [ ] TLS 1.2+ for all external connections
- [ ] Strong cipher suites only
- [ ] Certificate validation enabled
- [ ] CORS configured properly
- [ ] Firewall rules restrict access
- [ ] Network policies in Kubernetes
- [ ] Service mesh for mTLS (Istio/Linkerd)

**Kubernetes Security**:
- [ ] RBAC enabled and configured
- [ ] Pod Security Standards enforced (restricted)
- [ ] Network policies restrict pod-to-pod traffic
- [ ] Secrets encrypted at rest
- [ ] No privileged pods
- [ ] AppArmor/SELinux profiles applied
- [ ] Audit logging enabled
- [ ] Admission controllers configured (OPA/Kyverno)

---

### Network Security

**TLS Configuration**:
```cpp
#include <network_system/tls_server.hpp>

auto server = kcenon::TlsServer({
    .bind_address = "0.0.0.0",
    .port = 8443,

    // Certificate and key
    .cert_file = "/etc/ssl/certs/server.crt",
    .key_file = "/etc/ssl/private/server.key",
    .ca_file = "/etc/ssl/certs/ca.crt",  // Client cert validation

    // TLS version
    .min_version = kcenon::TlsVersion::TLS_1_2,
    .max_version = kcenon::TlsVersion::TLS_1_3,

    // Cipher suites (strong only)
    .ciphers = {
        "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",
        "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",
        "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305"
    },

    // Client certificate validation
    .verify_client = true,
    .verify_depth = 3
});

server.start();
```

**Kubernetes Network Policy**:
```yaml
# network-policy.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: my-app-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: my-app

  policyTypes:
  - Ingress
  - Egress

  # Ingress: only from ingress controller
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress-nginx
    ports:
    - protocol: TCP
      port: 8080

  # Egress: only to database and external APIs
  egress:
  # Allow DNS
  - to:
    - namespaceSelector: {}
    ports:
    - protocol: UDP
      port: 53

  # Allow database
  - to:
    - podSelector:
        matchLabels:
          app: postgres
    ports:
    - protocol: TCP
      port: 5432

  # Allow external HTTPS
  - to:
    - namespaceSelector: {}
    ports:
    - protocol: TCP
      port: 443
```

---

### Authentication and Authorization

**JWT Authentication**:
```cpp
#include <jwt-cpp/jwt.h>

class AuthMiddleware {
public:
    bool verify_token(const std::string& token) {
        try {
            auto decoded = jwt::decode(token);

            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{"secret"})
                .with_issuer("my-app")
                .with_audience("api");

            verifier.verify(decoded);

            // Check expiration
            auto exp = decoded.get_expires_at();
            if (exp < std::chrono::system_clock::now()) {
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            logger->error("Token verification failed", {{"error", e.what()}});
            return false;
        }
    }

    std::optional<std::string> get_user_id(const std::string& token) {
        auto decoded = jwt::decode(token);
        return decoded.get_payload_claim("user_id").as_string();
    }
};

// HTTP middleware
void handle_request(const Request& req) {
    auto auth_header = req.get_header("Authorization");
    if (!auth_header || !auth_header->starts_with("Bearer ")) {
        return HttpResponse(401, "Unauthorized");
    }

    auto token = auth_header->substr(7);  // Remove "Bearer "
    if (!auth_middleware->verify_token(token)) {
        return HttpResponse(401, "Invalid token");
    }

    auto user_id = auth_middleware->get_user_id(token);

    // ... process request with user_id ...
}
```

**RBAC (Role-Based Access Control)**:
```cpp
class AuthorizationService {
public:
    bool has_permission(const std::string& user_id, const std::string& resource, const std::string& action) {
        // Get user roles from database
        auto roles = db_->execute("SELECT role FROM user_roles WHERE user_id = ?", user_id);

        for (const auto& role : roles) {
            // Check if role has permission
            auto perms = db_->execute(
                "SELECT 1 FROM role_permissions "
                "WHERE role = ? AND resource = ? AND action = ?",
                role, resource, action
            );

            if (!perms.empty()) {
                return true;
            }
        }

        return false;
    }
};

// Check permission before action
if (!authz->has_permission(user_id, "orders", "delete")) {
    return HttpResponse(403, "Forbidden");
}
```

---

### Secrets Management

**Kubernetes Secrets**:
```yaml
# secret.yaml
apiVersion: v1
kind: Secret
metadata:
  name: app-secrets
  namespace: production
type: Opaque
stringData:
  DATABASE_PASSWORD: "super-secret-password"
  API_KEY: "api-key-value"
  JWT_SECRET: "jwt-signing-secret"
```

```yaml
# Use in deployment
env:
- name: DATABASE_PASSWORD
  valueFrom:
    secretKeyRef:
      name: app-secrets
      key: DATABASE_PASSWORD
```

**HashiCorp Vault Integration**:
```cpp
#include <vault/vault.hpp>

class SecretsManager {
public:
    std::string get_secret(const std::string& path) {
        // Authenticate with Vault
        auto vault_token = std::getenv("VAULT_TOKEN");
        auto vault_addr = std::getenv("VAULT_ADDR");

        vault::Client client(vault_addr, vault_token);

        // Read secret
        auto response = client.read_secret(path);
        return response["data"]["value"];
    }
};

// Usage
auto db_password = secrets->get_secret("database/production/password");
auto api_key = secrets->get_secret("external/api/key");
```

**External Secrets Operator (Kubernetes)**:
```yaml
# external-secret.yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-secrets
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend
    kind: SecretStore

  target:
    name: app-secrets
    creationPolicy: Owner

  data:
  - secretKey: DATABASE_PASSWORD
    remoteRef:
      key: database/production
      property: password

  - secretKey: API_KEY
    remoteRef:
      key: external/api
      property: key
```

---

## 7. Upgrade and Rollback

### Version Compatibility

**Semantic Versioning**:
```
MAJOR.MINOR.PATCH

MAJOR: Breaking changes (incompatible API changes)
MINOR: New features (backward compatible)
PATCH: Bug fixes (backward compatible)

Examples:
1.0.0 → 1.0.1  Safe (patch)
1.0.1 → 1.1.0  Safe (minor)
1.1.0 → 2.0.0  Breaking (major) - requires migration
```

**Compatibility Matrix**:
```
| Version | Compatible With | Breaking Changes | Migration Required |
|---------|----------------|------------------|-------------------|
| 1.0.x   | 1.0.0+         | None             | No                |
| 1.1.x   | 1.0.0+         | None             | No                |
| 2.0.x   | 2.0.0+         | Config schema    | Yes (config)      |
| 2.1.x   | 2.0.0+         | None             | No                |
```

**Upgrade Path**:
```
1.0.x → 1.1.x: Direct upgrade
1.0.x → 2.0.x: Requires config migration
1.0.x → 2.1.x: Upgrade to 2.0.x first, then 2.1.x
```

---

### Upgrade Procedures

**Pre-Upgrade Checklist**:
- [ ] Review changelog for breaking changes
- [ ] Backup database
- [ ] Backup configuration files
- [ ] Test upgrade in staging environment
- [ ] Notify users of planned maintenance (if downtime)
- [ ] Prepare rollback plan
- [ ] Monitor resources (disk space, memory)

**Blue-Green Deployment (Zero Downtime)**:
```bash
# Step 1: Deploy new version (green) alongside old (blue)
kubectl apply -f deployment-v2.yaml

# deployment-v2.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app-v2  # Different name
  labels:
    version: v2
spec:
  replicas: 3
  template:
    metadata:
      labels:
        app: my-app
        version: v2
    spec:
      containers:
      - name: app
        image: my-app:2.0.0  # New version

# Step 2: Wait for new pods to be ready
kubectl wait --for=condition=Ready pod -l version=v2 --timeout=300s

# Step 3: Switch traffic to new version
kubectl patch service my-app -p '{"spec":{"selector":{"version":"v2"}}}'

# Step 4: Monitor for errors
# If all good:
kubectl delete deployment my-app-v1  # Remove old version

# If errors:
kubectl patch service my-app -p '{"spec":{"selector":{"version":"v1"}}}'  # Rollback
```

**Rolling Update (Kubernetes Default)**:
```yaml
# deployment.yaml
spec:
  replicas: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 2        # Create 2 extra pods during update
      maxUnavailable: 1  # Max 1 pod can be unavailable

# Update image
kubectl set image deployment/my-app app=my-app:2.0.0

# Monitor rollout
kubectl rollout status deployment/my-app

# Pause rollout if issues
kubectl rollout pause deployment/my-app

# Resume
kubectl rollout resume deployment/my-app
```

**Database Migration**:
```bash
# Step 1: Backup database
pg_dump production_db > backup_$(date +%Y%m%d).sql

# Step 2: Run migrations
./migrate --database postgresql://user:pass@host/db up

# Step 3: Verify migration
./migrate --database postgresql://user:pass@host/db version

# Step 4: Deploy application

# Rollback migration if needed
./migrate --database postgresql://user:pass@host/db down 1
```

---

### Rollback Procedures

**Kubernetes Rollback**:
```bash
# View rollout history
kubectl rollout history deployment/my-app

# Rollback to previous version
kubectl rollout undo deployment/my-app

# Rollback to specific revision
kubectl rollout undo deployment/my-app --to-revision=3

# Check rollback status
kubectl rollout status deployment/my-app
```

**Database Rollback**:
```bash
# Restore from backup
psql production_db < backup_20250101.sql

# Or rollback specific migration
./migrate --database postgresql://user:pass@host/db down 1
```

**Configuration Rollback**:
```bash
# Git-based configuration (recommended)
# Rollback ConfigMap
kubectl apply -f config/v1.0.0/configmap.yaml

# Restart pods to pick up old config
kubectl rollout restart deployment/my-app
```

---

### Zero-Downtime Upgrades

**Strategy 1: Blue-Green Deployment**

See [Upgrade Procedures](#upgrade-procedures) above.

**Strategy 2: Canary Deployment**

```yaml
# Canary deployment (10% traffic to new version)
apiVersion: v1
kind: Service
metadata:
  name: my-app
spec:
  selector:
    app: my-app  # Matches both v1 and v2
  ports:
  - port: 80
    targetPort: 8080

---
# Old version (90% of pods)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app-v1
spec:
  replicas: 9  # 90%
  template:
    metadata:
      labels:
        app: my-app
        version: v1

---
# New version (10% of pods)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app-v2
spec:
  replicas: 1  # 10%
  template:
    metadata:
      labels:
        app: my-app
        version: v2

# Monitor metrics for canary
# If good: increase v2 replicas, decrease v1 replicas
# Repeat until 100% v2
```

**Strategy 3: Feature Flags**

```cpp
class FeatureFlags {
public:
    bool is_enabled(const std::string& feature, const std::string& user_id) {
        // Check if feature is enabled for user
        auto percentage = get_rollout_percentage(feature);
        auto user_hash = std::hash<std::string>{}(user_id) % 100;

        return user_hash < percentage;
    }

private:
    int get_rollout_percentage(const std::string& feature) {
        // Read from config or feature flag service
        return config_->get<int>("features." + feature + ".rollout_percentage", 0);
    }
};

// Usage
if (feature_flags->is_enabled("new_algorithm", user_id)) {
    return new_algorithm(data);  // New version
} else {
    return old_algorithm(data);  // Old version
}
```

---

## Related Documentation

- [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) - Cross-system integration patterns
- [performance/E2E_BENCHMARKS.md](performance/E2E_BENCHMARKS.md) - End-to-end performance benchmarks
- [PRODUCTION_QUALITY.md](PRODUCTION_QUALITY.md) - Production quality standards
- [ADAPTER_GUIDE.md](ADAPTER_GUIDE.md) - Adapter framework for service integration
- [ERROR_CODES.md](ERROR_CODES.md) - Error handling and codes

**System-Specific Documentation**:
- [thread_system/README.md](../../thread_system/README.md) - Thread pool configuration
- [logger_system/README.md](../../logger_system/README.md) - Logging configuration
- [database_system/README.md](../../database_system/README.md) - Database connection pooling
- [network_system/README.md](../../network_system/README.md) - Network server configuration
- [monitoring_system/README.md](../../monitoring_system/README.md) - Metrics and health checks

---

**Version**: 1.0.0
**Last Updated**: 2025-12-03
**Maintainer**: kcenon team
