
export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const path = url.pathname;

    // =========================================
    // ENVIRONMENT DIAGNOSTICS
    // =========================================

    if (path === "/envcheck") {
      const deviceToken =
        typeof env.DEVICE_TOKEN === "string"
          ? env.DEVICE_TOKEN
          : "";

      const viewerToken =
        typeof env.VIEWER_TOKEN === "string"
          ? env.VIEWER_TOKEN
          : "";

      return Response.json({
        worker: "exeed-can-cloud",
        deviceTokenExists: deviceToken.length > 0,
        deviceTokenLength: deviceToken.length,
        viewerTokenExists: viewerToken.length > 0,
        viewerTokenLength: viewerToken.length,
        canHubExists: !!env.CAN_HUB
      });
    }

    // =========================================
    // HEALTH CHECK
    // =========================================

    if (path === "/" || path === "/health") {
      return Response.json({
        status: "online",
        service: "EXEED CAN CLOUD",
        mode: "READ ONLY"
      });
    }

    // =========================================
    // DEVICE AUTHENTICATION
    // =========================================

    let role;

    if (path === "/device") {
      const auth =
        request.headers.get("Authorization") || "";

      const expected =
        typeof env.DEVICE_TOKEN === "string"
          ? env.DEVICE_TOKEN
          : "";

      const hasBearer = auth.startsWith("Bearer ");

      const receivedToken = hasBearer
        ? auth.slice(7)
        : "";

      const matches =
        expected.length > 0 &&
        receivedToken === expected;

      console.log("[ENV CHECK]", {
        deviceTokenExists: expected.length > 0,
        deviceTokenLength: expected.length,
        canHubExists: !!env.CAN_HUB
      });

      console.log("[AUTH] DEVICE request", {
        headerExists: auth.length > 0,
        bearerPrefix: hasBearer,
        receivedLength: receivedToken.length,
        expectedLength: expected.length,
        tokensMatch: matches
      });

      if (!matches) {
        console.log("[AUTH] DEVICE AUTH FAILED");

        return Response.json({
          error: "Unauthorized",
          diagnostic: {
            headerExists: auth.length > 0,
            bearerPrefix: hasBearer,
            receivedLength: receivedToken.length,
            expectedLength: expected.length,
            tokenPresent: expected.length > 0,
            canHubAvailable: !!env.CAN_HUB,
            tokensMatch: false
          }
        }, {
          status: 401
        });
      }

      console.log("[AUTH] DEVICE AUTH OK");

      role = "device";
    }

    // =========================================
    // VIEWER AUTHENTICATION
    // =========================================

    else if (path === "/viewer") {
      const received =
        url.searchParams.get("token") || "";

      const expected =
        typeof env.VIEWER_TOKEN === "string"
          ? env.VIEWER_TOKEN
          : "";

      if (!expected || received !== expected) {
        console.log("[AUTH] VIEWER AUTH FAILED");

        return new Response("Unauthorized", {
          status: 401
        });
      }

      console.log("[AUTH] VIEWER AUTH OK");

      role = "viewer";
    }

    // =========================================
    // STATUS
    // =========================================

    else if (path === "/status") {
      role = "status";
    }

    // =========================================
    // UNKNOWN ROUTE
    // =========================================

    else {
      return new Response("Not found", {
        status: 404
      });
    }

    // =========================================
    // WEBSOCKET VALIDATION
    // =========================================

    if (
      role !== "status" &&
      request.headers.get("Upgrade")?.toLowerCase() !== "websocket"
    ) {
      return new Response("WebSocket required", {
        status: 426
      });
    }

    // =========================================
    // DURABLE OBJECT CHECK
    // =========================================

    if (!env.CAN_HUB) {
      console.log("[ERROR] CAN_HUB binding missing");

      return Response.json({
        error: "CAN_HUB binding unavailable"
      }, {
        status: 500
      });
    }

    // =========================================
    // DURABLE OBJECT ROUTING
    // =========================================

    const id = env.CAN_HUB.idFromName("exeed-main");
    const stub = env.CAN_HUB.get(id);

    const internalUrl = new URL(request.url);

    internalUrl.pathname =
      role === "status"
        ? "/status"
        : "/connect";

    internalUrl.searchParams.set("role", role);

    return stub.fetch(
      new Request(internalUrl, request)
    );
  }
};


// =============================================
// DURABLE OBJECT: CAN HUB
// =============================================

export class CanHub {
  constructor(state, env) {
    this.state = state;
    this.env = env;

    this.totalFrames = 0;
    this.lastSeen = null;
  }

  // ===========================================
  // HTTP AND WEBSOCKET CONNECTION
  // ===========================================

  async fetch(request) {
    const url = new URL(request.url);

    // STATUS
    if (url.pathname === "/status") {
      return Response.json({
        status: "online",
        mode: "READ ONLY",
        viewers: this.state.getWebSockets("viewer").length,
        devices: this.state.getWebSockets("device").length,
        totalFrames: this.totalFrames,
        lastSeen: this.lastSeen
      });
    }

    // CONNECT
    if (url.pathname !== "/connect") {
      return new Response("Not found", {
        status: 404
      });
    }

    if (
      request.headers.get("Upgrade")?.toLowerCase() !== "websocket"
    ) {
      return new Response("WebSocket required", {
        status: 426
      });
    }

    const role = url.searchParams.get("role");

    if (role !== "device" && role !== "viewer") {
      return new Response("Invalid role", {
        status: 400
      });
    }

    const pair = new WebSocketPair();

    const client = pair[0];
    const server = pair[1];

    this.state.acceptWebSocket(server, [role]);

    server.serializeAttachment({
      role: role
    });

    console.log("[WS] Connected:", role);

    return new Response(null, {
      status: 101,
      webSocket: client
    });
  }

  // ===========================================
  // RECEIVE WEBSOCKET MESSAGE
  // ===========================================

  webSocketMessage(ws, message) {
    const attachment = ws.deserializeAttachment();
    const role = attachment?.role;

    // Only device can send data.
    if (role !== "device") {
      ws.close(1008, "Read only connection");
      return;
    }

    if (typeof message !== "string") {
      ws.close(1003, "Text JSON only");
      return;
    }

    let packet;

    try {
      packet = JSON.parse(message);
    } catch {
      ws.close(1007, "Invalid JSON");
      return;
    }

    if (
      packet?.type !== "frames" ||
      !Array.isArray(packet.frames) ||
      packet.frames.length > 500
    ) {
      ws.close(1008, "Invalid CAN packet");
      return;
    }

    // Update statistics.
    this.totalFrames += packet.frames.length;
    this.lastSeen = Date.now();

    // Prepare viewer packet.
    const outgoing = JSON.stringify({
      type: "frames",
      frames: packet.frames
    });

    // Broadcast to viewers only.
    for (const viewer of this.state.getWebSockets("viewer")) {
      try {
        viewer.send(outgoing);
      } catch (error) {
        console.log("[WS] Viewer send error");

        try {
          viewer.close(1011, "Broadcast error");
        } catch {}
      }
    }
  }

  // ===========================================
  // WEBSOCKET CLOSE
  // ===========================================

  webSocketClose(ws, code, reason) {
    console.log("[WS] Closed:", code, reason);
  }

  // ===========================================
  // WEBSOCKET ERROR
  // ===========================================

  webSocketError(ws, error) {
    console.log("[WS] Error:", error);
  }
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const path = url.pathname;

    // =========================================
    // HEALTH CHECK
    // =========================================

    if (path === "/" || path === "/health") {
      return Response.json({
        status: "online",
        service: "EXEED CAN CLOUD",
        mode: "READ ONLY"
      });
    }

    let role;

    // =========================================
    // DEVICE AUTHENTICATION
    // =========================================

    if (path === "/device") {
      const auth = request.headers.get("Authorization") || "";

      const expected =
        typeof env.DEVICE_TOKEN === "string"
          ? env.DEVICE_TOKEN
          : "";

      const hasHeader = auth.length > 0;
      const hasBearer = auth.startsWith("Bearer ");

      const receivedToken = hasBearer
        ? auth.slice(7)
        : "";

      const matches =
        expected.length > 0 &&
        receivedToken === expected;

      // Environment diagnostics
      console.log("[ENV CHECK]", {
        worker: "exeed-can-cloud",
        tokenPresent: expected.length > 0,
        tokenLength: expected.length,
        canHubAvailable: !!env.CAN_HUB
      });

      // Authentication diagnostics
      console.log("[AUTH] DEVICE request");
      console.log("[AUTH] Header exists:", hasHeader);
      console.log("[AUTH] Bearer prefix:", hasBearer);
      console.log("[AUTH] Received token length:", receivedToken.length);
      console.log("[AUTH] Expected token length:", expected.length);
      console.log("[AUTH] Tokens match:", matches);

      if (!matches) {
        console.log("[AUTH] DEVICE AUTH FAILED");

        return Response.json({
          error: "Unauthorized",
          diagnostic: {
            headerExists: hasHeader,
            bearerPrefix: hasBearer,
            receivedLength: receivedToken.length,
            expectedLength: expected.length,
            tokenPresent: expected.length > 0,
            canHubAvailable: !!env.CAN_HUB,
            tokensMatch: false
          }
        }, {
          status: 401
        });
      }

      console.log("[AUTH] DEVICE AUTH OK");

      role = "device";

    // =========================================
    // VIEWER AUTHENTICATION
    // =========================================

    } else if (path === "/viewer") {
      const received =
        url.searchParams.get("token") || "";

      const expected =
        typeof env.VIEWER_TOKEN === "string"
          ? env.VIEWER_TOKEN
          : "";

      if (!expected || received !== expected) {
        console.log("[AUTH] VIEWER AUTH FAILED");

        return new Response("Unauthorized", {
          status: 401
        });
      }

      console.log("[AUTH] VIEWER AUTH OK");

      role = "viewer";

    // =========================================
    // STATUS
    // =========================================

    } else if (path === "/status") {
      role = "status";

    } else {
      return new Response("Not found", {
        status: 404
      });
    }

    // =========================================
    // WEBSOCKET VALIDATION
    // =========================================

    if (
      role !== "status" &&
      request.headers.get("Upgrade")?.toLowerCase() !== "websocket"
    ) {
      return new Response("WebSocket required", {
        status: 426
      });
    }

    // =========================================
    // DURABLE OBJECT
    // =========================================

    if (!env.CAN_HUB) {
      console.log("[ERROR] CAN_HUB binding missing");

      return Response.json({
        error: "CAN_HUB binding unavailable"
      }, {
        status: 500
      });
    }

    const id = env.CAN_HUB.idFromName("exeed-main");
    const stub = env.CAN_HUB.get(id);

    const internalUrl = new URL(request.url);

    internalUrl.pathname =
      role === "status"
        ? "/status"
        : "/connect";

    internalUrl.searchParams.set("role", role);

    return stub.fetch(
      new Request(internalUrl, request)
    );
  }
};


// =============================================
// DURABLE OBJECT: CAN HUB
// =============================================

export class CanHub {
  constructor(state, env) {
    this.state = state;
    this.env = env;

    this.totalFrames = 0;
    this.lastSeen = null;
  }

  // ===========================================
  // HTTP / WEBSOCKET CONNECTION
  // ===========================================

  async fetch(request) {
    const url = new URL(request.url);

    if (url.pathname === "/status") {
      return Response.json({
        status: "online",
        mode: "READ ONLY",
        viewers: this.state.getWebSockets("viewer").length,
        devices: this.state.getWebSockets("device").length,
        totalFrames: this.totalFrames,
        lastSeen: this.lastSeen
      });
    }

    if (url.pathname !== "/connect") {
      return new Response("Not found", {
        status: 404
      });
    }

    if (
      request.headers.get("Upgrade")?.toLowerCase() !== "websocket"
    ) {
      return new Response("WebSocket required", {
        status: 426
      });
    }

    const role = url.searchParams.get("role");

    if (role !== "device" && role !== "viewer") {
      return new Response("Invalid role", {
        status: 400
      });
    }

    const pair = new WebSocketPair();

    const client = pair[0];
    const server = pair[1];

    this.state.acceptWebSocket(server, [role]);

    server.serializeAttachment({
      role: role
    });

    console.log("[WS] Connected:", role);

    return new Response(null, {
      status: 101,
      webSocket: client
    });
  }

  // ===========================================
  // WEBSOCKET MESSAGE
  // ===========================================

  webSocketMessage(ws, message) {
    const attachment = ws.deserializeAttachment();
    const role = attachment?.role;

    // Device is the only sender.
    if (role !== "device") {
      ws.close(1008, "Read only connection");
      return;
    }

    if (typeof message !== "string") {
      ws.close(1003, "Text JSON only");
      return;
    }

    let packet;

    try {
      packet = JSON.parse(message);
    } catch {
      ws.close(1007, "Invalid JSON");
      return;
    }

    if (
      packet?.type !== "frames" ||
      !Array.isArray(packet.frames) ||
      packet.frames.length > 500
    ) {
      ws.close(1008, "Invalid CAN packet");
      return;
    }

    this.totalFrames += packet.frames.length;
    this.lastSeen = Date.now();

    const outgoing = JSON.stringify({
      type: "frames",
      frames: packet.frames
    });

    // Broadcast only to read-only viewers.
    for (const viewer of this.state.getWebSockets("viewer")) {
      try {
        viewer.send(outgoing);
      } catch {
        try {
          viewer.close(1011, "Broadcast error");
        } catch {}
      }
    }
  }

  // ===========================================
  // WEBSOCKET CLOSE
  // ===========================================

  webSocketClose(ws, code, reason) {
    console.log("[WS] Closed:", code, reason);

    try {
      ws.close(code, reason);
    } catch {}
  }

  // ===========================================
  // WEBSOCKET ERROR
  // ===========================================

  webSocketError(ws) {
    console.log("[WS] Error");

    try {
      ws.close(1011, "WebSocket error");
    } catch {}
  }
}
