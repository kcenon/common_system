# Production Deployment Guide - Deployment Patterns

> **SSOT**: This document is part of the [Production Deployment Guide](PRODUCTION_GUIDE.md).

## 2. Deployment Patterns

### Pattern 1: Monolith Deployment

**Description**: All 7 systems in a single process/container.

**Pros**:
- Simple deployment and operations
- Low latency (in-process communication)
- Easy debugging and tracing
- No network overhead

**Cons**:
- Scaling all systems together (resource waste)
- Single point of failure
- Difficult to update individual systems

**Use Cases**:
- Small to medium applications (<10K RPS)
- Internal tools and dashboards
- Proof-of-concept deployments

**Architecture Diagram**:
```
┌─────────────────────────────────────────┐
│       Monolith Process                  │
│  ┌────────────────────────────────┐     │
│  │  Application Code              │     │
│  └────────────────────────────────┘     │
│            ▲                            │
│            │                            │
│  ┌─────────┴──────────────────────┐     │
│  │  Unified Bootstrapper          │     │
│  └────────────────────────────────┘     │
│            ▲                            │
│            │ initializes                │
│  ┌─────────┴──────────────────────┐     │
│  │  All 7 Systems (in-process)    │     │
│  │  • network_system               │     │
│  │  • database_system              │     │
│  │  • monitoring_system            │     │
│  │  • logger_system                │     │
│  │  • container_system             │     │
│  │  • thread_system                │     │
│  │  • common_system                │     │
│  └────────────────────────────────┘     │
└─────────────────────────────────────────┘
         │
         │ (single deployment unit)
         ▼
   [Container/VM/Bare Metal]
```

**Configuration Example**:
```yaml
# config/production.yaml
deployment:
  pattern: monolith

  # All systems configured in single file
  systems:
    - common
    - thread
    - container
    - logger
    - monitoring
    - database
    - network

  # Resource allocation
  resources:
    cpu: 4 cores
    memory: 8GB

  # Scaling
  replicas: 3  # Deploy 3 instances behind load balancer
```

**Deployment Steps**:
```bash
# 1. Build single binary
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target my_app

# 2. Create container image
docker build -t my-app:1.0.0 .

# 3. Deploy to production
docker run -d \
  --name my-app \
  -p 8080:8080 \
  -v /etc/app/config:/etc/app/config:ro \
  -v /var/log/app:/var/log/app \
  --restart unless-stopped \
  my-app:1.0.0
```

**Monitoring**:
```yaml
# Prometheus scrape config
scrape_configs:
  - job_name: 'monolith'
    static_configs:
      - targets: ['app-01:9091', 'app-02:9091', 'app-03:9091']
```

---

### Pattern 2: Microservice Deployment

**Description**: Each system (or group of systems) in separate processes/containers.

**Pros**:
- Independent scaling per system
- Fault isolation (logger crash doesn't affect database)
- Technology flexibility (different languages per service)
- Team autonomy

**Cons**:
- Network latency between services
- Complex deployment and orchestration
- Distributed tracing required
- Higher operational overhead

**Use Cases**:
- Large-scale applications (>100K RPS)
- Polyglot architectures
- Teams with microservice expertise

**Architecture Diagram**:
```
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  API Gateway │  │  Web Service │  │ Worker Service│
└──────┬───────┘  └──────┬───────┘  └──────┬───────┘
       │                 │                 │
       │ HTTP/gRPC       │ HTTP/gRPC       │ Message Queue
       ▼                 ▼                 ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│Network Service│  │Database Svc  │  │Logger Service│
│(network_sys)  │  │(database_sys)│  │(logger_sys)  │
└──────────────┘  └──────────────┘  └──────────────┘
       │                 │                 │
       │                 │                 │
       ▼                 ▼                 ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│Thread Service │  │Container Svc │  │Monitor Svc   │
│(thread_sys)   │  │(container_sys)│ │(monitor_sys) │
└──────────────┘  └──────────────┘  └──────────────┘
                         │
                         │ (all depend on)
                         ▼
                  ┌──────────────┐
                  │Common Service│
                  │(common_sys)  │
                  └──────────────┘
```

**Service Grouping Strategy**:
```yaml
# Group services by tier
services:
  # API tier (stateless)
  - name: api-service
    systems: [network, logger, thread, common]
    replicas: 10

  # Data tier (stateful)
  - name: database-service
    systems: [database, logger, thread, common]
    replicas: 3

  # Background jobs tier
  - name: worker-service
    systems: [container, logger, thread, common]
    replicas: 5

  # Observability tier
  - name: monitoring-service
    systems: [monitoring, logger, thread, common]
    replicas: 2
```

**Inter-Service Communication**:
```cpp
// Use network_system for service-to-service calls
#include <network_system/http_client.hpp>

// Call database service from API service
auto db_client = kcenon::HttpClient("http://database-service:8081");
auto response = db_client.post("/query", query_data);

// Use service mesh (Istio/Linkerd) for:
// - mTLS encryption
// - Load balancing
// - Circuit breaking
// - Retries and timeouts
```

**Deployment (Kubernetes)**:
```yaml
# api-service.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-service
spec:
  replicas: 10
  template:
    spec:
      containers:
      - name: api
        image: my-app/api-service:1.0.0
        ports:
        - containerPort: 8080
        env:
        - name: DATABASE_SERVICE_URL
          value: "http://database-service:8081"
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: 2000m
            memory: 2Gi

---
# database-service.yaml
apiVersion: apps/v1
kind: StatefulSet  # Stateful for persistent storage
metadata:
  name: database-service
spec:
  replicas: 3
  serviceName: database-service
  template:
    spec:
      containers:
      - name: database
        image: my-app/database-service:1.0.0
        volumeMounts:
        - name: data
          mountPath: /var/lib/app/data
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Gi
```

---

### Pattern 3: Sidecar Deployment

**Description**: Common systems (logger, monitoring) as sidecars, business logic in main container.

**Pros**:
- Separation of concerns (business logic vs. infrastructure)
- Shared infrastructure code across applications
- Easier to update infrastructure without touching app code
- Standard observability across all apps

**Cons**:
- Higher resource usage (multiple containers per pod)
- Inter-container communication overhead
- More complex pod configuration

**Use Cases**:
- Kubernetes environments with service mesh
- Standardized logging/monitoring across organization
- Legacy applications needing observability

**Architecture Diagram**:
```
┌─────────────────────────────────────────────┐
│              Kubernetes Pod                 │
│                                             │
│  ┌─────────────────────────────────────┐    │
│  │   Main Application Container        │    │
│  │  (Business Logic + network/db/thread)│   │
│  └─────────────────────────────────────┘    │
│            │                                │
│            │ stdout/stderr                  │
│            │ HTTP (metrics)                 │
│            ▼                                │
│  ┌─────────────────────────────────────┐    │
│  │   Logger Sidecar Container          │    │
│  │  • Collects logs from main app      │    │
│  │  • Forwards to log aggregator       │    │
│  │  • File rotation and buffering      │    │
│  └─────────────────────────────────────┘    │
│            │                                │
│            ▼ (send to Elasticsearch)       │
│                                             │
│  ┌─────────────────────────────────────┐    │
│  │  Monitoring Sidecar Container       │    │
│  │  • Scrapes /metrics from main app   │    │
│  │  • Forwards to Prometheus           │    │
│  │  • Health check proxy               │    │
│  └─────────────────────────────────────┘    │
│            │                                │
│            ▼ (send to Prometheus)          │
└─────────────────────────────────────────────┘
```

**Pod Configuration**:
```yaml
# sidecar-pod.yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-sidecars
spec:
  containers:
  # Main application
  - name: app
    image: my-app:1.0.0
    ports:
    - containerPort: 8080
      name: http
    - containerPort: 9091
      name: metrics
    volumeMounts:
    - name: logs
      mountPath: /var/log/app
    resources:
      requests:
        cpu: 1000m
        memory: 2Gi
      limits:
        cpu: 2000m
        memory: 4Gi

  # Logger sidecar
  - name: logger
    image: logger-sidecar:1.0.0
    volumeMounts:
    - name: logs
      mountPath: /var/log/app
      readOnly: true
    env:
    - name: LOG_DESTINATION
      value: "elasticsearch:9200"
    - name: LOG_INDEX
      value: "app-logs"
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi

  # Monitoring sidecar (Prometheus exporter)
  - name: metrics-exporter
    image: prometheus-exporter:1.0.0
    ports:
    - containerPort: 9090
      name: exporter-metrics
    env:
    - name: APP_METRICS_URL
      value: "http://localhost:9091/metrics"
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi

  # Shared volume for logs
  volumes:
  - name: logs
    emptyDir: {}
```

**Sidecar Implementation**:
```cpp
// Main application exposes metrics endpoint
#include <network_system/http_server.hpp>

void setup_metrics_endpoint() {
    auto server = kcenon::HttpServer(9091);

    // Expose metrics in Prometheus format
    server.route("/metrics", [](const auto& req) {
        std::stringstream metrics;
        metrics << "# HELP app_requests_total Total requests\n";
        metrics << "# TYPE app_requests_total counter\n";
        metrics << "app_requests_total " << get_request_count() << "\n";

        return kcenon::HttpResponse(200, metrics.str());
    });

    server.start();
}
```

---

### Pattern 4: Hybrid Deployment

**Description**: Combination of monolith + microservices + sidecars.

**Use Cases**:
- Migration from monolith to microservices
- Gradual extraction of services
- Different scaling needs per component

**Architecture Example**:
```
┌─────────────────────────────────────┐
│     Monolith (Core Business Logic)  │  ← Most traffic here
│  (all 7 systems)                    │
└─────────────┬───────────────────────┘
              │
              │ Calls specialized services
              │
    ┌─────────┼─────────┬─────────────┐
    │         │         │             │
    ▼         ▼         ▼             ▼
┌──────┐  ┌──────┐  ┌────────┐  ┌─────────┐
│ML Svc│  │Search│  │Billing │  │ Logger  │ ← Extracted services
│      │  │ Svc  │  │  Svc   │  │ Sidecar │
└──────┘  └──────┘  └────────┘  └─────────┘
```

**Migration Strategy**:
1. **Start**: Monolith with all systems
2. **Extract**: High-load systems (e.g., search, ML inference)
3. **Stabilize**: Run hybrid for months
4. **Iterate**: Extract more services based on need
5. **End goal**: Microservices (optional)

---

## 3. Container Deployment

### Docker Configuration

**Multi-Stage Dockerfile**:
```dockerfile
# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    g++-11 \
    cmake \
    ninja-build \
    git \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
WORKDIR /app
COPY . .

# Build application (static linking for portability)
RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-11 \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    -G Ninja
RUN cmake --build build --target my_app

# Stage 2: Runtime environment (minimal)
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN groupadd -r appuser && useradd -r -g appuser appuser

# Copy binary from builder
COPY --from=builder /app/build/my_app /usr/local/bin/my_app

# Copy configuration
COPY config/production.yaml /etc/app/config.yaml

# Create log directory
RUN mkdir -p /var/log/app && chown appuser:appuser /var/log/app

# Switch to non-root user
USER appuser

# Expose ports
EXPOSE 8080 9090 9091

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:9090/health || exit 1

# Run application
ENTRYPOINT ["/usr/local/bin/my_app"]
CMD ["--config", "/etc/app/config.yaml"]
```

**Build and Push**:
```bash
# Build image
docker build -t my-app:1.0.0 .

# Tag for registry
docker tag my-app:1.0.0 registry.example.com/my-app:1.0.0
docker tag my-app:1.0.0 registry.example.com/my-app:latest

# Push to registry
docker push registry.example.com/my-app:1.0.0
docker push registry.example.com/my-app:latest
```

**Docker Compose (Local Testing)**:
```yaml
# docker-compose.yml
version: '3.8'

services:
  app:
    image: my-app:1.0.0
    ports:
      - "8080:8080"   # Application
      - "9090:9090"   # Health check
      - "9091:9091"   # Metrics
    volumes:
      - ./config:/etc/app/config:ro
      - logs:/var/log/app
    environment:
      - LOG_LEVEL=info
      - DB_HOST=postgres
    depends_on:
      - postgres
      - redis
    restart: unless-stopped

  postgres:
    image: postgres:15
    environment:
      - POSTGRES_DB=production_db
      - POSTGRES_USER=app_user
      - POSTGRES_PASSWORD=secret
    volumes:
      - pgdata:/var/lib/postgresql/data

  redis:
    image: redis:7
    volumes:
      - redisdata:/data

  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9092:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - prometheus-data:/prometheus

  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    volumes:
      - grafana-data:/var/lib/grafana

volumes:
  logs:
  pgdata:
  redisdata:
  prometheus-data:
  grafana-data:
```

---

### Kubernetes Deployment

**Namespace Setup**:
```yaml
# namespace.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    environment: production
```

**ConfigMap for Configuration**:
```yaml
# configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: production
data:
  production.yaml: |
    common:
      error_handling:
        detailed_errors: false

    thread:
      pool_size: 16

    logger:
      default_level: info
      async:
        enabled: true

    monitoring:
      prometheus:
        enabled: true
        port: 9091

    # ... (rest of config)
```

**Secret for Sensitive Data**:
```yaml
# secret.yaml
apiVersion: v1
kind: Secret
metadata:
  name: app-secrets
  namespace: production
type: Opaque
stringData:
  DB_PASSWORD: "super-secret-password"
  API_KEY: "api-key-value"
```

**Deployment**:
```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: production
  labels:
    app: my-app
    version: v1.0.0
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0  # Zero-downtime deployment

  selector:
    matchLabels:
      app: my-app

  template:
    metadata:
      labels:
        app: my-app
        version: v1.0.0
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "9091"
        prometheus.io/path: "/metrics"

    spec:
      # Security context
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 1000

      containers:
      - name: app
        image: registry.example.com/my-app:1.0.0
        imagePullPolicy: IfNotPresent

        ports:
        - name: http
          containerPort: 8080
          protocol: TCP
        - name: health
          containerPort: 9090
          protocol: TCP
        - name: metrics
          containerPort: 9091
          protocol: TCP

        # Environment variables
        env:
        - name: LOG_LEVEL
          value: "info"
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: app-secrets
              key: DB_PASSWORD
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace

        # Volume mounts
        volumeMounts:
        - name: config
          mountPath: /etc/app/config
          readOnly: true
        - name: logs
          mountPath: /var/log/app

        # Resource limits
        resources:
          requests:
            cpu: 1000m      # 1 CPU core
            memory: 2Gi
          limits:
            cpu: 2000m      # 2 CPU cores
            memory: 4Gi

        # Liveness probe (is container alive?)
        livenessProbe:
          httpGet:
            path: /health
            port: 9090
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 3
          failureThreshold: 3

        # Readiness probe (can it serve traffic?)
        readinessProbe:
          httpGet:
            path: /health
            port: 9090
          initialDelaySeconds: 5
          periodSeconds: 5
          timeoutSeconds: 2
          failureThreshold: 2

        # Graceful shutdown
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 15"]  # Allow time for connection draining

      # Volumes
      volumes:
      - name: config
        configMap:
          name: app-config
      - name: logs
        emptyDir: {}

      # Pod anti-affinity (spread across nodes)
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: my-app
              topologyKey: kubernetes.io/hostname
```

**Service (Load Balancer)**:
```yaml
# service.yaml
apiVersion: v1
kind: Service
metadata:
  name: my-app
  namespace: production
spec:
  type: LoadBalancer
  selector:
    app: my-app
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  sessionAffinity: ClientIP  # Sticky sessions
```

**Horizontal Pod Autoscaler**:
```yaml
# hpa.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-app-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70  # Scale when CPU > 70%
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80  # Scale when memory > 80%
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # Wait 5 min before scaling down
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60  # Max 50% scale-down per minute
```

**Deployment Commands**:
```bash
# Create namespace
kubectl apply -f namespace.yaml

# Create ConfigMap and Secret
kubectl apply -f configmap.yaml
kubectl apply -f secret.yaml

# Deploy application
kubectl apply -f deployment.yaml
kubectl apply -f service.yaml
kubectl apply -f hpa.yaml

# Check status
kubectl -n production get pods
kubectl -n production get svc
kubectl -n production get hpa

# View logs
kubectl -n production logs -f deployment/my-app

# Scale manually
kubectl -n production scale deployment my-app --replicas=5
```

---

### Container Optimization

**Image Size Optimization**:
```dockerfile
# Use Alpine for minimal size (trade-off: musl libc compatibility)
FROM alpine:3.18 AS runtime

# Or use distroless (no shell, smallest attack surface)
FROM gcr.io/distroless/cc-debian11
COPY --from=builder /app/build/my_app /app
ENTRYPOINT ["/app"]
```

**Build Cache Optimization**:
```dockerfile
# Order layers from least to most frequently changing
# 1. OS dependencies (changes rarely)
RUN apt-get update && apt-get install -y ...

# 2. Third-party libraries (changes occasionally)
COPY third_party/ /app/third_party/
RUN cd /app/third_party && make install

# 3. Application dependencies (changes frequently)
COPY CMakeLists.txt /app/
RUN cmake -B /app/build ...

# 4. Source code (changes most frequently)
COPY src/ /app/src/
RUN cmake --build /app/build
```

**Security Hardening**:
```dockerfile
# Scan image for vulnerabilities
# Use tools: trivy, snyk, clair

# Run as non-root
USER 1000:1000

# Read-only root filesystem
# (requires writable /tmp and /var/log via volumes)
```

**Resource Limits (Kubernetes)**:
```yaml
resources:
  requests:
    cpu: "1"       # Guaranteed CPU
    memory: 2Gi    # Guaranteed memory
  limits:
    cpu: "2"       # Max CPU (can burst up to 2 cores)
    memory: 4Gi    # Max memory (OOMKilled if exceeded)
```

---

**Version**: 1.0.0
**Last Updated**: 2025-12-03
**Maintainer**: kcenon team
