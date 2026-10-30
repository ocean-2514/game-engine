#include "UI/UIContext.h"
#include "UI/UIButton.h"
#include "Renderer/Texture.h"
#include <limits>
#include "Renderer/RenderQueue.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace eng;
namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
struct Probe : UIElement {
    int layouts = 0;
    std::vector<UIPointerEvent> events;
    std::function<void(const UIPointerEvent&)> pointerCallback;
    std::function<void()> layoutCallback;
    std::function<void()> visibilityCallback;
    std::function<void()> submitCallback;
    void OnLayout() override { ++layouts; if (layoutCallback) layoutCallback(); }
    void OnPointerEvent(const UIPointerEvent& event) override {
        events.push_back(event);
        if (pointerCallback) pointerCallback(event);
    }
    void OnVisibilityChanged(bool) override { if (visibilityCallback) visibilityCallback(); }
    void OnSubmit(UIDrawList&) const override { if (submitCallback) submitCallback(); }
};
ScreenViewDesc View() { return {200, 100, ScreenOrigin::TopLeft, {0, 0, 400, 200}}; }
WindowMetrics Metrics() { return {200, 100, 400, 200}; }
InputEvent Event(InputEventType type, float x, float y) {
    InputEvent event;
    event.type = type;
    event.key = Key::MouseLeft;
    event.mouse = {x, y};
    return event;
}
std::unique_ptr<Probe> Element(UIRect rect) {
    auto result = std::make_unique<Probe>();
    result->SetRect(rect);
    return result;
}
int Count(const Probe& probe, UIPointerEventType type) {
    int count = 0;
    for (const auto& event : probe.events) if (event.type == type) ++count;
    return count;
}

void LayoutAndHandles() {
    UIContext context{};
    context.SetView(View());
    auto root = context.AddRoot(Element({10, 20, 100, 50}));
    auto child = context.AttachElement(Element({3, 4, 10, 8}), root);
    context.Layout();
    Check(context.Resolve(child)->GetArrangedRect().x == 13, "child x");
    Check(context.Resolve(child)->GetArrangedRect().y == 24, "child y");
    context.Resolve(root)->SetRect({20, 30, 100, 50});
    context.Layout();
    Check(context.Resolve(child)->GetArrangedRect().x == 23, "parent dirtiness propagation");
    auto* probe = static_cast<Probe*>(context.Resolve(child));
    const int before = probe->layouts;
    context.Layout();
    Check(probe->layouts == before, "clean layout must not invoke callbacks");
    context.Resolve(child)->SetVisibility(UIVisibility::Collapsed);
    context.Layout();
    Check(context.Resolve(child)->GetArrangedRect().width == 0, "collapsed size");
    context.Resolve(child)->SetVisibility(UIVisibility::Visible);
    context.Layout();
    Check(context.Resolve(child)->GetArrangedRect().width == 10, "restored size");
    Check(context.Remove(child), "remove child");
    Check(!context.Resolve(child), "deleted handle invalid");
    auto replacement = context.AttachElement(Element({0, 0, 1, 1}), root);
    Check(replacement.index == child.index && replacement.generation != child.generation,
        "generation changes on reuse");
    Check(context.Remove(root) && !context.Resolve(replacement), "recursive removal");
    Check(!context.Remove({20, 0}), "forged handle rejected");
    for (int i = 0; i < 200; ++i) {
        auto handle = context.AddRoot(Element({}));
        Check(context.Resolve(handle) != nullptr && context.Remove(handle), "slot churn");
    }
}

void InputAndCapture() {
    UIContext context{};
    context.SetView(View());
    auto first = context.AddRoot(Element({0, 0, 30, 30}));
    auto second = context.AddRoot(Element({0, 0, 30, 30}));
    auto* a = static_cast<Probe*>(context.Resolve(first));
    auto* b = static_cast<Probe*>(context.Resolve(second));
    auto capture = context.ProcessInput({Event(InputEventType::MouseButtonPressed, 10, 10),
        Event(InputEventType::MouseMoved, 100, 80),
        Event(InputEventType::MouseButtonReleased, 100, 80)}, Metrics());
    Check(capture.pointer && !capture.keyboard, "UI capture flags");
    Check(Count(*a, UIPointerEventType::Down) == 0, "last sibling is topmost");
    Check(Count(*b, UIPointerEventType::Down) == 1 && Count(*b, UIPointerEventType::Up) == 1,
        "same-frame press/release and outside capture");
    a->SetRenderOrder(5);
    context.ProcessInput({Event(InputEventType::MouseButtonPressed, 10, 10)}, Metrics());
    Check(Count(*a, UIPointerEventType::Down) == 1, "render order also controls hit testing");
    a->SetEnabled(false);
    Check(Count(*a, UIPointerEventType::Cancel) == 1, "disabling cancels immediately");
    context.ProcessInput({Event(InputEventType::MouseButtonPressed, 10, 10)}, Metrics());
    context.CancelPointerInput();
    Check(Count(*b, UIPointerEventType::Cancel) == 1, "explicit cancellation");
    Check(!context.ProcessInput({}, {0, 0, 0, 0}).pointer, "minimized window");
    b->pointerCallback = [&](const UIPointerEvent& event) {
        if (event.type == UIPointerEventType::Down) context.CancelPointerInput();
    };
    const int cancels = Count(*b, UIPointerEventType::Cancel);
    context.ProcessInput({Event(InputEventType::MouseButtonPressed, 10, 10)}, Metrics());
    Check(Count(*b, UIPointerEventType::Cancel) == cancels + 1, "cancel inside a callback");
}

void SubViewportAndVisibility() {
    UIContext context{};
    context.SetView({100, 50, ScreenOrigin::TopLeft, {100, 50, 200, 100}});
    auto root = context.AddRoot(Element({0, 0, 100, 50}));
    auto* probe = static_cast<Probe*>(context.Resolve(root));
    context.ProcessInput({Event(InputEventType::MouseMoved, 75, 40)}, Metrics());
    const auto& event = probe->events.back();
    Check(std::abs(event.viewPosition.x - 25) < 0.001f &&
        std::abs(event.viewPosition.y - 15) < 0.001f, "DPI and subviewport mapping");
    context.SetView({100, 50, ScreenOrigin::BottomLeft, {100, 50, 200, 100}});
    context.ProcessInput({Event(InputEventType::MouseMoved, 75, 40)}, Metrics());
    Check(std::abs(probe->events.back().viewPosition.y - 35) < 0.001f, "bottom-left mapping");
    auto child = context.AttachElement(Element({0, 0, 20, 20}), root);
    context.Resolve(root)->SetVisibility(UIVisibility::Hidden);
    Check(!context.ProcessInput({Event(InputEventType::MouseMoved, 55, 70)}, Metrics()).pointer,
        "hidden parent suppresses subtree");
    context.Resolve(root)->SetVisibility(UIVisibility::Visible);
    context.Resolve(root)->SetHitTestVisible(false);
    context.Resolve(child)->SetRect({0, 0, 100, 50});
    Check(context.ProcessInput({Event(InputEventType::MouseMoved, 75, 40)}, Metrics()).pointer,
        "non-hit-testable container does not disable children");
}

void DeferredMutations() {
    UIContext context{};
    context.SetView(View());
    auto root = context.AddRoot(Element({0, 0, 100, 50}));
    auto* probe = static_cast<Probe*>(context.Resolve(root));
    UIElementHandle stagedRoot, stagedChild;
    bool once = false;
    probe->layoutCallback = [&] {
        if (once) return;
        once = true;
        stagedRoot = context.AddRoot(Element({0, 0, 10, 10}));
        Check(context.Resolve(stagedRoot) == nullptr, "staged handle not resolvable");
        stagedChild = context.AttachElement(Element({1, 1, 2, 2}), stagedRoot);
        Check(context.Remove(stagedChild), "staged removal accepted");
    };
    context.Layout();
    Check(context.Resolve(stagedRoot) && !context.Resolve(stagedChild), "ordered commands");
    probe->pointerCallback = [&](const UIPointerEvent& event) {
        if (event.type == UIPointerEventType::Down) context.Remove(root);
    };
    context.Resolve(stagedRoot)->SetHitTestVisible(false);
    context.ProcessInput({Event(InputEventType::MouseButtonPressed, 20, 20),
        Event(InputEventType::MouseButtonReleased, 20, 20)}, Metrics());
    Check(!context.Resolve(root), "self removal during input");
    // A removed parent must not leave a staged child slot reserved forever.
    auto parent = context.AddRoot(Element({0, 0, 10, 10}));
    auto* p = static_cast<Probe*>(context.Resolve(parent));
    UIElementHandle failedChild;
    p->layoutCallback = [&] {
        context.Remove(parent);
        failedChild = context.AttachElement(Element({}), parent);
    };
    context.Layout();
    Check(!context.Resolve(parent) && !context.Resolve(failedChild), "parent deleted before attach");
    auto reused = context.AddRoot(Element({}));
    Check(reused.index == failedChild.index && reused.generation != failedChild.generation,
        "failed attach releases reserved slot");
    auto* r = static_cast<Probe*>(context.Resolve(reused));
    r->visibilityCallback = [&] { context.Remove(reused); };
    r->SetVisibility(UIVisibility::Hidden);
    Check(!context.Resolve(reused), "visibility callback can remove itself safely");
}

struct ImageProbe : UIImage {
    UIDrawList Draw() const { UIDrawList list; OnSubmit(list); return list; }
};
struct ButtonProbe : UIButton {
    UIDrawList Draw() const { UIDrawList list; OnSubmit(list); return list; }
    void Send(const UIPointerEvent& event) { OnPointerEvent(event); }
};
struct TestTexture : Texture {
    bool IsValid() const override { return true; }
};

void Images() {
    UIContext context(View());
    auto parent = context.AddRoot(Element({30, 20, 100, 60}));
    auto image = std::make_unique<ImageProbe>();
    auto* probe = image.get();
    image->SetRect({5, 8, 20, 10});
    image->SetTint({0.2f, 0.4f, 0.6f, 0.8f});
    const auto tint = image->GetTint();
    Check(tint.g == 0.4f, "valid image tint accepted");
    auto invalid = tint;
    invalid.g = std::numeric_limits<float>::quiet_NaN();
    image->SetTint(invalid);
    Check(image->GetTint() == tint, "invalid tint rejected");
    image->SetUV({0.1f, 0.2f}, {0.7f, 0.8f});
    context.AttachElement(std::move(image), parent);
    context.Layout();
    const auto draw = probe->Draw();
    Check(draw.size() == 1 && draw[0].modelMatrix[3][0] == 45 &&
        draw[0].modelMatrix[3][1] == 33, "image uses arranged rectangle");
    Check(draw[0].color == tint && draw[0].lowerLeftUV == glm::vec2(0.1f, 0.2f),
        "image forwards tint and UV");
    probe->SetRect({0, 0, 0, 10});
    context.Layout();
    Check(probe->Draw().empty(), "zero sized image skipped");
}

void Buttons() {
    UIContext context(View());
    auto parent = context.AddRoot(Element({30, 20, 100, 60}));
    auto button = std::make_unique<ButtonProbe>();
    auto* probe = button.get();
    button->SetRect({5, 8, 20, 10});
    const auto texture = std::make_shared<TestTexture>();
    {
        UIImage background(texture);
        background.SetTint({0.3f, 0.4f, 0.5f, 0.6f});
        background.SetUV({0.1f, 0.2f}, {0.8f, 0.9f});
        button->SetBackground(&background);
    }
    auto handle = context.AttachElement(std::move(button), parent);
    context.Layout();
    auto draw = probe->Draw();
    Check(draw.size() == 1 && draw[0].texture == texture && draw[0].color.a == 0.6f,
        "background snapshot survives source destruction and preserves tint");
    int clicks = 0;
    probe->AddClickListener({});
    probe->AddClickListener([&] { ++clicks; });
    auto click = [&](float upX, float upY) {
        context.ProcessInput({Event(InputEventType::MouseButtonPressed, 40, 30),
            Event(InputEventType::MouseButtonReleased, upX, upY)}, Metrics());
    };
    click(40, 30);
    Check(clicks == 1, "nested button click");
    click(40, 38);
    Check(clicks == 1, "bottom edge excluded consistently");
    context.ProcessInput({Event(InputEventType::MouseButtonPressed, 40, 30)}, Metrics());
    probe->SetEnabled(false);
    probe->SetEnabled(true);
    context.ProcessInput({Event(InputEventType::MouseButtonReleased, 40, 30)}, Metrics());
    Check(clicks == 1, "cancelled press never clicks after re-enable");
    context.ProcessInput({Event(InputEventType::MouseButtonPressed, 40, 30),
        Event(InputEventType::MouseMoved, 150, 90),
        Event(InputEventType::MouseButtonReleased, 40, 30)}, Metrics());
    Check(clicks == 2, "capture survives leave and re-entry");
    UIPointerEvent up;
    up.type = UIPointerEventType::Up;
    up.button = Key::MouseLeft;
    up.viewPosition = {40, 30};
    probe->Send(up);
    Check(clicks == 2, "up without down does not click");
    probe->SetRect({150, 8, 40, 10}); // Arranged x=180: partly outside the view.
    context.ProcessInput({Event(InputEventType::MouseButtonPressed, 190, 30),
        Event(InputEventType::MouseButtonReleased, 210, 30)}, Metrics());
    Check(clicks == 2, "release outside view does not click even inside control rectangle");
    probe->SetRect({5, 8, 20, 10});
    int late = 0;
    probe->ClearClickListeners();
    probe->AddClickListener([&] {
        probe->ClearClickListeners();
        probe->AddClickListener([&] { ++late; });
    });
    probe->AddClickListener([&] { ++clicks; });
    click(40, 30);
    Check(clicks == 3 && late == 0, "listener mutation uses notification snapshot");
    click(40, 30);
    Check(late == 1, "new listener starts on next click");
    probe->ClearClickListeners();
    int accumulated = 0;
    probe->AddClickListener([&, count = 0]() mutable { accumulated = ++count; });
    click(40, 30);
    click(40, 30);
    Check(accumulated == 2, "mutable listener state survives snapshots");
    probe->ClearClickListeners();
    probe->AddClickListener([&] { context.Remove(handle); });
    click(40, 30);
    Check(!context.Resolve(handle), "button may delete itself in click listener");
}

void Submission() {
    UIContext context{};
    context.SetView(View());
    std::vector<int> order;
    auto a = context.AddRoot(Element({}));
    auto b = context.AddRoot(Element({}));
    auto child = context.AttachElement(Element({}), a);
    static_cast<Probe*>(context.Resolve(a))->submitCallback = [&] { order.push_back(1); };
    static_cast<Probe*>(context.Resolve(b))->submitCallback = [&] { order.push_back(2); };
    static_cast<Probe*>(context.Resolve(child))->submitCallback = [&] { order.push_back(3); };
    context.Resolve(b)->SetRenderOrder(-1);
    RenderQueue queue;
    context.Submit(queue);
    Check(order == std::vector<int>({2, 1, 3}), "stable paint traversal order");
    order.clear();
    context.Resolve(a)->SetVisibility(UIVisibility::Hidden);
    context.Submit(queue);
    Check(order == std::vector<int>({2}), "hidden subtree not submitted");
}
}

int main() {
    try {
        LayoutAndHandles();
        InputAndCapture();
        SubViewportAndVisibility();
        DeferredMutations();
        Submission();
        Images();
        Buttons();
        std::cout << "UI: 7 regression suites passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
